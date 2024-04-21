#ifndef RUNNERS_H
#define RUNNERS_H

#include <catch2/catch_message.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "docboy/core/core.h"
#include "docboy/gameboy/gameboy.h"
#include "extra/serial/endpoints/buffer.h"
#include "framebuffers.h"
#include "utils/formatters.hpp"
#include "utils/os.h"
#include "utils/path.h"

constexpr Lcd::Palette GREY_PALETTE {0xFFFF, 0xAD55, 0x52AA, 0x0000}; // {0xFF, 0xAA, 0x55, 0x00} in RGB565

constexpr uint64_t DURATION_VERY_LONG = 250'000'000;
constexpr uint64_t DURATION_LONG = 100'000'000;
constexpr uint64_t DURATION_MEDIUM = 30'000'000;
constexpr uint64_t DURATION_SHORT = 5'000'000;
constexpr uint64_t DURATION_VERY_SHORT = 1'500'000;
constexpr uint64_t DEFAULT_DURATION = DURATION_LONG;

template <typename RunnerImpl>
class Runner {
public:
    struct JoypadInput {
        uint64_t ticks {};
        Joypad::KeyState state {};
        Joypad::Key key {};
    };

    explicit Runner(const Lcd::Palette& palette = Lcd::DEFAULT_PALETTE) :
        gb {std::make_unique<GameBoy>(palette)},
        core {*gb} {
    }

    RunnerImpl& rom(const std::string& filename) {
        romName = filename;
        core.loadRom(filename);
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& maxTicks(uint64_t ticks) {
        maxTicks_ = ticks;
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& checkIntervalTicks(uint64_t ticks) {
        checkIntervalTicks_ = ticks / 4 * 4; // should be a multiple of 4
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& stopAtInstruction(std::optional<uint8_t> instr) {
        stopAtInstruction_ = instr;
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& forceCheck(bool force) {
        forceCheck_ = force;
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& scheduleInputs(std::vector<JoypadInput> inputs) {
        inputs_ = std::move(inputs);
        return static_cast<RunnerImpl&>(*this);
    }

    bool run() {
        canRun = true;

        auto* impl = static_cast<RunnerImpl*>(this);
        impl->onRun();

        bool hasEverChecked {false};

        for (tick = core.ticks; tick <= maxTicks_ && canRun; tick += 4) {
            // Eventually submit scheduled Joypad input.
            if (!inputs_.empty()) {
                for (auto it = inputs_.begin(); it != inputs_.end(); it++) {
                    if (tick >= it->ticks) {
                        core.setKey(it->key, it->state);
                        inputs_.erase(it);
                        break;
                    }
                }
            }

            // Advance emulation.
            core.cycle();

            // Check expectation.
            if (impl->shouldCheckExpectation()) {
                hasEverChecked = true;
                if (impl->checkExpectation()) {
                    return true;
                }
            }
        }

        if (hasEverChecked) {
            impl->onExpectationFailed();
        } else if (impl->shouldEverCheckExpectation()) {
            UNSCOPED_INFO("Expectation never checked!");
        }

        return false;
    }

    std::unique_ptr<GameBoy> gb {};
    Core core;

protected:
    std::string romName;
    uint64_t tick {};
    uint64_t maxTicks_ {UINT64_MAX};
    uint64_t checkIntervalTicks_ {UINT64_MAX};
    std::optional<uint8_t> stopAtInstruction_ {};
    bool forceCheck_ {};

    std::vector<JoypadInput> inputs_ {};

    bool canRun {true};
};

class SimpleRunner : public Runner<SimpleRunner> {
public:
    void onRun() {
    }
    bool shouldEverCheckExpectation() {
        return false;
    }
    bool shouldCheckExpectation() {
        return false;
    }
    bool checkExpectation() {
        return true;
    }
    void onExpectationFailed() {
    }
};

class FramebufferRunner : public Runner<FramebufferRunner> {
public:
    FramebufferRunner(const Lcd::Palette& palette) :
        Runner<FramebufferRunner>(palette) {
    }

    FramebufferRunner& expectFramebuffer(const std::string& filename) {
        load_framebuffer_png(filename, expectedFramebuffer);
        return *this;
    }

    void onRun() {
    }

    bool shouldEverCheckExpectation() {
        return true;
    }

    bool shouldCheckExpectation() {
        // Schedule a check for the next VBlank if we reached the required instruction
        if (stopAtInstruction_) {
            if (gb->cpu.instruction.opcode == *stopAtInstruction_) {
                // Force check now
                canRun = false;
                return true;
            }
        }

        // Schedule a check for the next VBlank if we have passed the check interval
        if (tick > 0 && tick % checkIntervalTicks_ == 0) {
            if (forceCheck_) {
                // Force check even if outside VBlank.
                return true;
            } else {
                pendingCheckNextVBlank = true;
            }
        }

        // Check if we are in VBlank with a pending check
        if (pendingCheckNextVBlank && keep_bits<2>(gb->video.STAT) == Specs::Ppu::Modes::VBLANK) {
            pendingCheckNextVBlank = false;
            return true;
        }

        return false;
    }

    bool checkExpectation() {
        memcpy(lastFramebuffer, gb->lcd.getPixels(), FRAMEBUFFER_SIZE);
        return are_framebuffer_equals(lastFramebuffer, expectedFramebuffer);
    }

    void onExpectationFailed() {
        // Framebuffer not equals: figure out where's the (first) problem
        UNSCOPED_INFO("=== " << romName << " ===");

        uint32_t i;
        for (i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
            if (lastFramebuffer[i] != expectedFramebuffer[i]) {
                UNSCOPED_INFO("Framebuffer mismatch at position "
                              << i << " (row=" << i / 160 << ", column=" << i % 160 << ": (actual) 0x"
                              << hex(lastFramebuffer[i]) << " != (expected) 0x" << hex(expectedFramebuffer[i]));
                break;
            }
        }
        check(i < FRAMEBUFFER_NUM_PIXELS);

        // Dump framebuffers

        const auto tmpPath = temp_directory_path() / "docboy";
        create_directory(tmpPath.string());

        const auto pathActual = (tmpPath / path {path {romName}.filename() + "-actual.png"}).string();
        const auto pathExpected = (tmpPath / path {path {romName}.filename() + "-expected.png"}).string();
        save_framebuffer_png(pathActual, lastFramebuffer);
        save_framebuffer_png(pathExpected, expectedFramebuffer);
        UNSCOPED_INFO("You can find the PNGs of the framebuffers at " << pathActual << " and " << pathExpected);
    }

private:
    uint16_t lastFramebuffer[FRAMEBUFFER_NUM_PIXELS] {};
    uint16_t expectedFramebuffer[FRAMEBUFFER_NUM_PIXELS] {};
    bool pendingCheckNextVBlank {};
};

class SerialRunner : public Runner<SerialRunner> {
public:
    SerialRunner& expectOutput(const std::vector<uint8_t>& output) {
        expectedOutput = output;
        return *this;
    }

    void onRun() {
        serialLink.plug1.attach(serialBuffer);
        core.attachSerialLink(serialLink.plug2);
    }

    bool shouldEverCheckExpectation() {
        return true;
    }

    bool shouldCheckExpectation() {
        return tick > 0 && tick % checkIntervalTicks_ == 0;
    }

    bool checkExpectation() {
        lastOutput = serialBuffer.buffer;
        return serialBuffer.buffer == expectedOutput;
    }

    void onExpectationFailed() {
        UNSCOPED_INFO("=== " << romName << " ===");
        UNSCOPED_INFO("Expected serial output: " << hex(expectedOutput));
        UNSCOPED_INFO("Actual serial output  : " << hex(lastOutput));
    }

private:
    SerialBuffer serialBuffer;
    SerialLink serialLink;
    std::vector<uint8_t> lastOutput;
    std::vector<uint8_t> expectedOutput;
};

struct MaxTicks {
    uint64_t value;
};
struct StopAtInstruction {
    uint8_t instruction;
};
struct ForceCheck {};

using Inputs = std::vector<FramebufferRunner::JoypadInput>;

struct FramebufferRunnerParams {
    using Param = std::variant<std::monostate, Lcd::Palette, MaxTicks, StopAtInstruction, ForceCheck,
                               std::vector<FramebufferRunner::JoypadInput>>;

    FramebufferRunnerParams(std::string&& rom, std::string&& expected, Param param1 = std::monostate {},
                            Param param2 = std::monostate {}) :
        rom(std::move(rom)) {
        result = std::move(expected);

        auto parseParam = [this](Param& param) {
            if (std::holds_alternative<Lcd::Palette>(param)) {
                palette = std::get<Lcd::Palette>(param);
            } else if (std::holds_alternative<MaxTicks>(param)) {
                maxTicks = std::get<MaxTicks>(param).value;
            } else if (std::holds_alternative<StopAtInstruction>(param)) {
                stopAtInstruction = std::get<StopAtInstruction>(param).instruction;
            } else if (std::holds_alternative<ForceCheck>(param)) {
                forceCheck = true;
            } else if (std::holds_alternative<std::vector<FramebufferRunner::JoypadInput>>(param)) {
                inputs = std::get<std::vector<FramebufferRunner::JoypadInput>>(param);
            }
        };

        parseParam(param1);
        parseParam(param2);
    }

    std::string rom;
    std::string result;
    std::optional<Lcd::Palette> palette {};
    uint64_t maxTicks {DEFAULT_DURATION};
    std::optional<uint8_t> stopAtInstruction {};
    bool forceCheck {};
    std::vector<FramebufferRunner::JoypadInput> inputs {};
};

struct SerialRunnerParams {
    SerialRunnerParams(std::string&& rom, std::vector<uint8_t>&& expected, uint64_t maxTicks_ = DURATION_MEDIUM) :
        rom(std::move(rom)) {
        result = std::move(expected);
        maxTicks = maxTicks_;
    }

    std::string rom;
    std::vector<uint8_t> result;
    uint64_t maxTicks;
};

#endif // RUNNERS_H