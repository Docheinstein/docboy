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

    bool run() {
        auto* impl = static_cast<RunnerImpl*>(this);
        impl->onRun();

        bool hasEverChecked {false};

        for (tick = core.ticks; tick <= maxTicks_ && canRun; tick += 4) {
            core.cycle();
            if (impl->shouldCheckExpectation()) {
                hasEverChecked = true;
                if (impl->checkExpectation()) {
                    return true;
                }
            }
        }

        if (hasEverChecked) {
            impl->onExpectationFailed();
        } else if (impl->shouldCheckExpectation()) {
            UNSCOPED_INFO("Expectation never checked!");
        }

        return false;
    }

    std::unique_ptr<GameBoy> gb {std::make_unique<GameBoy>()};
    Core core {*gb};

protected:
    std::string romName; // debug
    uint64_t tick {};
    uint64_t maxTicks_ {UINT64_MAX};
    uint64_t checkIntervalTicks_ {UINT64_MAX};
    std::optional<uint8_t> stopAtInstruction_ {};

    bool canRun {true};
};

class SimpleRunner : public Runner<SimpleRunner> {
public:
    void onRun() {
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
    FramebufferRunner& expectFramebuffer(const std::string& filename, const std::optional<Lcd::Palette>& palette_) {
        load_framebuffer_png(filename, expectedFramebuffer);
        if (palette_)
            convert_framebuffer_with_palette(expectedFramebuffer, *palette_, expectedFramebuffer, Lcd::DEFAULT_PALETTE);
        return *this;
    }

    void onRun() {
    }

    bool shouldCheckExpectation() {
        // Schedule a check for the next VBlank if we reached the required instruction
        if (stopAtInstruction_) {
            if (core.gb.cpu.instruction.opcode == *stopAtInstruction_) {
                // Force check now
                canRun = false;
                return true;
            }
        }

        // Schedule a check for the next VBlank if we have passed the check interval
        if (tick > 0 && tick % checkIntervalTicks_ == 0) {
            pendingCheck = true;
        }

        // Check if we are in VBlank with a pending check
        if (pendingCheck && keep_bits<2>(core.gb.video.STAT) == 1) {
            pendingCheck = false;
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
    bool pendingCheck {};
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
using RunnerParam = std::variant<std::monostate, Lcd::Palette, MaxTicks, StopAtInstruction>;

struct FramebufferRunnerParams {

    FramebufferRunnerParams(std::string&& rom, std::string&& expected, RunnerParam param1 = std::monostate {}) :
        rom(std::move(rom)) {
        result = std::move(expected);

        if (std::holds_alternative<Lcd::Palette>(param1)) {
            palette = std::get<Lcd::Palette>(param1);
        } else if (std::holds_alternative<MaxTicks>(param1)) {
            maxTicks = std::get<MaxTicks>(param1).value;
        } else if (std::holds_alternative<StopAtInstruction>(param1)) {
            stopAtInstruction = std::get<StopAtInstruction>(param1).instruction;
        }
    }

    std::string rom;
    std::string result;
    std::optional<Lcd::Palette> palette {};
    uint64_t maxTicks {DEFAULT_DURATION};
    std::optional<uint8_t> stopAtInstruction {};
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