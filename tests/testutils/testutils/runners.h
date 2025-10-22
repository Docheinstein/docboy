#ifndef RUNNERS_H
#define RUNNERS_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "docboy/bootrom/factory.h"
#include "docboy/core/core.h"
#include "docboy/gameboy/gameboy.h"

#include "extra/serial/endpoints/buffer.h"

#include "testutils/framebuffers.h"

#include "utils/formatters.h"
#include "utils/os.h"
#include "utils/path.h"
#include "utils/strings.h"

struct JoypadInput {
    uint64_t tick {};
    Joypad::Key key {};
    Joypad::KeyState state {};
};
struct ColorTolerance {
    uint8_t red {};
    uint8_t green {};
    uint8_t blue {};
};
struct CheckIntervalTicks {
    uint64_t value {};
};
struct MaxTicks {
    uint64_t value {};
};
struct StopAtInstruction {
    uint8_t instruction {};
};
struct MemoryExpectation {
    uint16_t address {};
    uint8_t success_value {};
    std::optional<uint8_t> fail_value {};
};
struct ForceCheck {};

constexpr uint64_t DEFAULT_CHECK_INTERVAL = 100'000;
constexpr uint64_t DEFAULT_MAX_DURATION = 100'000'000;

#ifdef ENABLE_CGB
constexpr ColorTolerance COLOR_TOLERANCE_LOW {5, 5, 5};
constexpr ColorTolerance COLOR_TOLERANCE_MEDIUM {10, 10, 10};
constexpr ColorTolerance COLOR_TOLERANCE_INSANE {150, 150, 150};
#endif

#ifndef ENABLE_CGB
constexpr Appearance GREY_PALETTE {0xFFFF, {0xFFFF, 0xAD55, 0x52AA, 0x0000}};
#endif

#ifdef ENABLE_BOOTROM
extern std::string boot_rom;
#endif

// TODO: tests with bootrom for CGB
#ifdef ENABLE_BOOTROM
constexpr uint64_t BOOT_DURATION = 23'440'324;
#else
constexpr uint64_t BOOT_DURATION = 0;
#endif

inline std::function<void(const std::string&)> make_default_runner_log() {
    return [](const std::string& message) {
        std::cerr << message << std::endl << std::flush;
    };
}

inline std::function<void(const std::string&)> runner_log = make_default_runner_log();

template <typename RunnerImpl>
class Runner {
public:
    enum class ExpectationResult { Success, Fail, Fatal };

    explicit Runner() :
        gb {std::make_unique<GameBoy>()},
        core {*gb} {
#ifdef ENABLE_BOOTROM
        core.load_boot_rom(boot_rom);
#endif
        core.set_audio_sample_rate(32786);
    }

    RunnerImpl& rom(const std::string& filename) {
        rom_name = filename;
        core.load_rom(filename);
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& appearance(const Appearance* appearance) {
        if (appearance) {
            gb->lcd.set_appearance(*appearance);
        }
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& max_ticks(uint64_t ticks) {
        max_ticks_ = ticks;
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& check_interval_ticks(uint64_t ticks) {
        check_interval_ticks_ = ticks / 4 * 4; // should be a multiple of 4
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& stop_at_instruction(std::optional<uint8_t> instr) {
        stop_at_instruction_ = instr;
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& force_check(bool force) {
        force_check_ = force;
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& limit_speed(bool limit) {
        limit_speed_ = limit;
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& schedule_inputs(std::vector<JoypadInput> inputs) {
        inputs_ = std::move(inputs);
        // Eventually add BOOT_DURATION time to inputs timing
        for (auto& in : inputs_) {
            in.tick += BOOT_DURATION;
        }
        return static_cast<RunnerImpl&>(*this);
    }

    bool run() {
        can_run = true;

        auto* impl = static_cast<RunnerImpl*>(this);
        impl->on_run();

        bool has_ever_checked {false};

        static constexpr std::chrono::nanoseconds FRAME_TIME {1000000000LLU * Specs::Ppu::DOTS_PER_FRAME /
                                                              Specs::Frequencies::CLOCK};
        std::chrono::high_resolution_clock::time_point next_frame_time =
            std::chrono::high_resolution_clock::now() + FRAME_TIME;
        uint64_t frame_ticks = 0;

        for (tick = core.ticks; tick <= max_ticks_ && can_run; tick += 4) {
            // Eventually submit scheduled Joypad input.
            if (!inputs_.empty()) {
                for (auto it = inputs_.begin(); it != inputs_.end(); ++it) {
                    if (tick >= it->tick) {
                        core.set_key(it->key, it->state);
                        inputs_.erase(it);
                        break;
                    }
                }
            }

            // Advance emulation.
            try {
                core.cycle();
            } catch (const std::runtime_error& err) {
                throw std::runtime_error(rom_name + "\n" + err.what());
            }

            impl->on_cycle();

            // Check expectation.
            if (impl->should_check_expectation()) {
                has_ever_checked = true;
                ExpectationResult result = impl->check_expectation();
                if (result == ExpectationResult::Success) {
                    return true;
                }
                if (result == ExpectationResult::Fatal) {
                    // Certainly a failure, no way the tests will succeed later.
                    break;
                }
            }

            if (limit_speed_) {
                // Eventually run at real speed, not faster (this is useful for RTC tests).
                if (frame_ticks == Specs::Ppu::DOTS_PER_FRAME) {
                    frame_ticks = 0;
                    while (std::chrono::high_resolution_clock::now() < next_frame_time) { /* busy wait */
                    }
                    next_frame_time += FRAME_TIME;
                }

                frame_ticks += 4;
            }
        }

        if (has_ever_checked) {
            impl->on_expectation_failed();
        } else if (impl->should_ever_check_expectation()) {
            runner_log(rom_name);
            runner_log("Expectation never checked!");
        }

        return false;
    }

    std::unique_ptr<GameBoy> gb {};
    Core core;

protected:
    std::string rom_name;
    uint64_t tick {};
    uint64_t max_ticks_ {UINT64_MAX};
    uint64_t check_interval_ticks_ {UINT64_MAX};
    std::optional<uint8_t> stop_at_instruction_ {};
    bool force_check_ {};
    bool limit_speed_ {};

    std::vector<JoypadInput> inputs_ {};

    bool can_run {true};
};

class SimpleRunner : public Runner<SimpleRunner> {
public:
    SimpleRunner() :
        Runner<SimpleRunner>() {
    }

    void on_run() {
    }

    void on_cycle() {
    }

    bool should_ever_check_expectation() {
        return false;
    }

    bool should_check_expectation() {
        return false;
    }

    ExpectationResult check_expectation() {
        ASSERT_NO_ENTRY();
        return ExpectationResult::Fail;
    }

    void on_expectation_failed() {
    }
};

class FramebufferRunner : public Runner<FramebufferRunner> {
public:
    explicit FramebufferRunner() :
        Runner<FramebufferRunner>() {
    }

    FramebufferRunner& expect_framebuffer(const std::string& filename) {
        bool ok = load_framebuffer_png(filename, expected_framebuffer);
        if (!ok) {
            FATAL("failed to read file '" + filename + "'");
        }
        return *this;
    }

    FramebufferRunner& color_tolerance(uint8_t red, uint8_t green, uint8_t blue) {
        color_tolerance_.red = red;
        color_tolerance_.green = green;
        color_tolerance_.blue = blue;
        return *this;
    }

    void on_run() {
    }

    void on_cycle() {
    }

    bool should_ever_check_expectation() {
        return true;
    }

    bool should_check_expectation() {
        // Schedule a check for the next VBlank if we reached the required instruction
        if (stop_at_instruction_) {
            if (gb->cpu.instruction.opcode == *stop_at_instruction_) {
                // Force check now
                can_run = false;
                return true;
            }
        }

        // Schedule a check for the next VBlank if we have passed the check interval
        if (tick > 0 && tick % check_interval_ticks_ == 0) {
            if (force_check_) {
                // Force check even if outside VBlank.
                return true;
            } else {
                pending_check_next_vblank = true;
            }
        }

        // Check if we are in VBlank with a pending check
        if (pending_check_next_vblank && gb->ppu.stat.mode == Specs::Ppu::Modes::VBLANK) {
            pending_check_next_vblank = false;
            return true;
        }

        return false;
    }

    ExpectationResult check_expectation() {
        memcpy(last_framebuffer, gb->lcd.get_pixels(), FRAMEBUFFER_SIZE);

        bool framebuffer_match = false;

        if (color_tolerance_.red == 0 && color_tolerance_.green == 0 && color_tolerance_.blue == 0) {
            framebuffer_match = are_framebuffer_equals(last_framebuffer, expected_framebuffer);
        } else {
            framebuffer_match =
                are_framebuffer_equals_with_tolerance(last_framebuffer, expected_framebuffer, color_tolerance_.red,
                                                      color_tolerance_.green, color_tolerance_.blue);
        }

        return framebuffer_match ? ExpectationResult::Success : ExpectationResult::Fail;
    }

    void on_expectation_failed() {
        // Framebuffer not equals: figure out where's the (first) problem
        std::stringstream output_message;

        output_message << rom_name << std::endl;

        uint32_t i;
        for (i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
            if (!are_pixel_equals_with_tolerance(last_framebuffer[i], expected_framebuffer[i], color_tolerance_.red,
                                                 color_tolerance_.green, color_tolerance_.blue)) {
                int32_t dr, dg, db;
                compute_pixels_delta(last_framebuffer[i], expected_framebuffer[i], dr, dg, db);

                output_message << "Framebuffer mismatch" << std::endl
                               << "Position   : " << i << std::endl
                               << "Row        : " << i / 160 << std::endl
                               << "Column     : " << i % 160 << std::endl
                               << "Actual     : 0x" << hex(last_framebuffer[i]) << std::endl
                               << "Expected   : 0x" << hex(expected_framebuffer[i]) << std::endl
                               << "Delta      : (" << dr << ", " << dg << ", " << db << ")" << std::endl
                               << "Tolerance  : (" << +color_tolerance_.red << ", " << +color_tolerance_.green << ", "
                               << +color_tolerance_.blue << ")" << std::endl;
                break;
            }
        }
        ASSERT(i < FRAMEBUFFER_NUM_PIXELS);

        // Dump framebuffers

        const auto tmp_path = temp_directory_path() / "docboy";
        create_directory(tmp_path.string());

        const auto path_actual = (tmp_path / Path {Path {rom_name}.filename() + "-actual.png"}).string();
        const auto path_expected = (tmp_path / Path {Path {rom_name}.filename() + "-expected.png"}).string();
        save_framebuffer_png(path_actual, last_framebuffer);
        save_framebuffer_png(path_expected, expected_framebuffer);
        output_message << "You can find the PNGs of the framebuffers at " << path_actual << " and " << path_expected;

        runner_log(output_message.str());
    }

private:
    uint16_t last_framebuffer[FRAMEBUFFER_NUM_PIXELS] {};
    uint16_t expected_framebuffer[FRAMEBUFFER_NUM_PIXELS] {};
    bool pending_check_next_vblank {};
    struct {
        uint8_t red {0};
        uint8_t green {0};
        uint8_t blue {0};
    } color_tolerance_;
};

class SerialRunner : public Runner<SerialRunner> {
public:
    explicit SerialRunner() :
        Runner<SerialRunner>() {
    }

    SerialRunner& expect_output(const std::vector<uint8_t>& output) {
        expected_output = output;
        return *this;
    }

    void on_run() {
        core.attach_serial_link(serial_buffer);
    }

    void on_cycle() {
    }

    bool should_ever_check_expectation() {
        return true;
    }

    bool should_check_expectation() {
        return tick > 0 && tick % check_interval_ticks_ == 0;
    }

    ExpectationResult check_expectation() {
        last_output = serial_buffer.buffer;
        return serial_buffer.buffer == expected_output ? ExpectationResult::Success : ExpectationResult::Fail;
    }

    void on_expectation_failed() {
        runner_log(rom_name);
        runner_log("Expected serial output: " + hex(expected_output));
        runner_log("Actual serial output  : " + hex(last_output));
    }

private:
    SerialBuffer serial_buffer;
    std::vector<uint8_t> last_output;
    std::vector<uint8_t> expected_output;
};

class MemoryRunner : public Runner<MemoryRunner> {
public:
    explicit MemoryRunner() :
        Runner<MemoryRunner>() {
    }

    MemoryRunner& expect_output(const std::vector<MemoryExpectation>& output) {
        expected_output = output;
        return *this;
    }

    void on_run() {
    }

    void on_cycle() {
    }

    bool should_ever_check_expectation() {
        return true;
    }

    bool should_check_expectation() {
        return tick > 0 && tick % check_interval_ticks_ == 0;
    }

    ExpectationResult check_expectation() {
        bool success = true;
        bool fatal = false;

        last_output.clear();

        for (const auto& [address, success_value, fail_value] : expected_output) {
            uint8_t real_value = gb->mmu.bus_accessors[Device::Cpu][address].read_bus(address);
            fatal = fatal || (fail_value && (real_value == *fail_value));
            success = success && (real_value == success_value);
            last_output.emplace_back(address, real_value);
        }

        return success ? ExpectationResult::Success : (fatal ? ExpectationResult::Fatal : ExpectationResult::Fail);
    }

    void on_expectation_failed() {
        runner_log(rom_name);

        std::string expected_output_str = "[" +
                                          join(expected_output, ",",
                                               [](const MemoryExpectation& expectation) {
                                                   std::stringstream str;
                                                   str << "{" << hex(expectation.address) << ", "
                                                       << hex(expectation.success_value) << "}";
                                                   return str.str();
                                               }) +
                                          "]";
        std::string last_output_str = "[" +
                                      join(last_output, ",",
                                           [](const std::pair<uint16_t, uint8_t>& address_to_value) {
                                               std::stringstream str;
                                               str << "{" << hex(address_to_value.first) << ", "
                                                   << hex(address_to_value.second) << "}";
                                               return str.str();
                                           }) +
                                      "]";

        runner_log("Expected output: " + expected_output_str);
        runner_log("Actual output  : " + last_output_str);
    }

private:
    std::vector<std::pair<uint16_t, uint8_t>> last_output;
    std::vector<MemoryExpectation> expected_output;
};

class TwoPlayersFramebufferRunner : public Runner<TwoPlayersFramebufferRunner> {
public:
    explicit TwoPlayersFramebufferRunner() :
        Runner<TwoPlayersFramebufferRunner>(),
        gb2 {std::make_unique<GameBoy>()},
        core2 {*gb2} {
#ifdef ENABLE_BOOTROM
        core2.load_boot_rom(boot_rom);
#endif
        core2.set_audio_sample_rate(32786);
    }

    TwoPlayersFramebufferRunner& rom2(const std::string& filename) {
        rom_name2 = filename;
        core2.load_rom(filename);
        return *this;
    }

    TwoPlayersFramebufferRunner& expect_framebuffer1(const std::string& filename) {
        bool ok = load_framebuffer_png(filename, expected_framebuffer1);
        if (!ok) {
            FATAL("failed to read file '" + filename + "'");
        }
        return *this;
    }

    TwoPlayersFramebufferRunner& expect_framebuffer2(const std::string& filename) {
        bool ok = load_framebuffer_png(filename, expected_framebuffer2);
        if (!ok) {
            FATAL("failed to read file '" + filename + "'");
        }
        return *this;
    }

    TwoPlayersFramebufferRunner& color_tolerance(uint8_t red, uint8_t green, uint8_t blue) {
        color_tolerance_.red = red;
        color_tolerance_.green = green;
        color_tolerance_.blue = blue;
        return *this;
    }

    void on_run() {
        core.attach_serial_link(gb2->serial);
        core2.attach_serial_link(gb->serial);
    }

    void on_cycle() {
        core2.cycle();
    }

    bool should_ever_check_expectation() {
        return true;
    }

    bool should_check_expectation() {
        // Schedule a check for the next VBlank if we have passed the check interval
        if (tick > 0 && tick % check_interval_ticks_ == 0) {
            pending_check_next_vblank = true;
        }

        // Check if we are in VBlank with a pending check
        if (pending_check_next_vblank && gb->ppu.stat.mode == Specs::Ppu::Modes::VBLANK) {
            pending_check_next_vblank = false;
            return true;
        }

        return false;
    }

    ExpectationResult check_expectation() {
        memcpy(last_framebuffer1, gb->lcd.get_pixels(), FRAMEBUFFER_SIZE);
        memcpy(last_framebuffer2, gb2->lcd.get_pixels(), FRAMEBUFFER_SIZE);

        bool framebuffer_match = false;

        if (color_tolerance_.red == 0 && color_tolerance_.green == 0 && color_tolerance_.blue == 0) {
            framebuffer_match = are_framebuffer_equals(last_framebuffer1, expected_framebuffer1) &&
                                are_framebuffer_equals(last_framebuffer2, expected_framebuffer2);
        } else {
            framebuffer_match =
                are_framebuffer_equals_with_tolerance(last_framebuffer1, expected_framebuffer1, color_tolerance_.red,
                                                      color_tolerance_.green, color_tolerance_.blue) &&
                are_framebuffer_equals_with_tolerance(last_framebuffer1, expected_framebuffer2, color_tolerance_.red,
                                                      color_tolerance_.green, color_tolerance_.blue);
        }

        return framebuffer_match ? ExpectationResult::Success : ExpectationResult::Fail;
    }

    void on_expectation_failed() {
        // Framebuffer not equals: figure out where's the (first) problem
        std::stringstream output_message;

        output_message << rom_name << "," << rom_name2 << std::endl;

        const auto compare_framebuffers = [&output_message, this](uint16_t* last_framebuffer,
                                                                  uint16_t* expected_framebuffer) {
            uint32_t i;
            for (i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
                if (!are_pixel_equals_with_tolerance(last_framebuffer[i], expected_framebuffer[i], color_tolerance_.red,
                                                     color_tolerance_.green, color_tolerance_.blue)) {
                    int32_t dr, dg, db;
                    compute_pixels_delta(last_framebuffer[i], expected_framebuffer[i], dr, dg, db);

                    output_message << "Framebuffer mismatch" << std::endl
                                   << "Position   : " << i << std::endl
                                   << "Row        : " << i / 160 << std::endl
                                   << "Column     : " << i % 160 << std::endl
                                   << "Actual     : 0x" << hex(last_framebuffer[i]) << std::endl
                                   << "Expected   : 0x" << hex(expected_framebuffer[i]) << std::endl
                                   << "Delta      : (" << dr << ", " << dg << ", " << db << ")" << std::endl
                                   << "Tolerance  : (" << +color_tolerance_.red << ", " << +color_tolerance_.green
                                   << ", " << +color_tolerance_.blue << ")" << std::endl;
                    return false;
                }
            }

            return true;
        };

        bool ok1 = compare_framebuffers(last_framebuffer1, expected_framebuffer1);
        bool ok2 = compare_framebuffers(last_framebuffer2, expected_framebuffer2);

        // Dump framebuffers

        const auto tmp_path = temp_directory_path() / "docboy";
        create_directory(tmp_path.string());

        if (!ok1) {
            const auto path_actual = (tmp_path / Path {Path {rom_name}.filename() + "-actual.png"}).string();
            const auto path_expected = (tmp_path / Path {Path {rom_name}.filename() + "-expected.png"}).string();
            save_framebuffer_png(path_actual, last_framebuffer1);
            save_framebuffer_png(path_expected, expected_framebuffer1);
            output_message << "You can find the PNGs of the framebuffers at " << path_actual << " and " << path_expected
                           << std::endl;
        }

        if (!ok2) {
            const auto path_actual = (tmp_path / Path {Path {rom_name2}.filename() + "-actual.png"}).string();
            const auto path_expected = (tmp_path / Path {Path {rom_name2}.filename() + "-expected.png"}).string();
            save_framebuffer_png(path_actual, last_framebuffer2);
            save_framebuffer_png(path_expected, expected_framebuffer2);
            output_message << "You can find the PNGs of the framebuffers at " << path_actual << " and " << path_expected
                           << std::endl;
        }

        runner_log(output_message.str());
    }

private:
    std::unique_ptr<GameBoy> gb2 {};
    Core core2;

    std::string rom_name2 {};

    uint16_t last_framebuffer1[FRAMEBUFFER_NUM_PIXELS] {};
    uint16_t last_framebuffer2[FRAMEBUFFER_NUM_PIXELS] {};
    uint16_t expected_framebuffer1[FRAMEBUFFER_NUM_PIXELS] {};
    uint16_t expected_framebuffer2[FRAMEBUFFER_NUM_PIXELS] {};
    bool pending_check_next_vblank {};
    struct {
        uint8_t red {0};
        uint8_t green {0};
        uint8_t blue {0};
    } color_tolerance_;
};

using Inputs = std::vector<JoypadInput>;

struct BaseRunnerParams {
    std::string rom {};
    uint64_t check_interval_ticks {DEFAULT_CHECK_INTERVAL};
    uint64_t max_ticks {DEFAULT_MAX_DURATION};
    std::optional<uint8_t> stop_at_instruction {};
    bool force_check {};
    bool limit_speed {};
    std::vector<JoypadInput> inputs {};
};

struct FramebufferRunnerParams : BaseRunnerParams {
    FramebufferRunnerParams(std::string&& rom, std::string&& expected) :
        result {std::move(expected)} {
        this->rom = std::move(rom);
    }

    std::string result;

    const Appearance* appearance {};
    ColorTolerance color_tolerance {};
};

struct SerialRunnerParams : BaseRunnerParams {
    SerialRunnerParams(std::string&& rom, std::vector<uint8_t>&& expected) :
        result {std::move(expected)} {
        this->rom = std::move(rom);
    }

    std::vector<uint8_t> result;
};

struct MemoryRunnerParams : BaseRunnerParams {
    MemoryRunnerParams(std::string&& rom, std::vector<MemoryExpectation>&& expected) :
        result {std::move(expected)} {
        this->rom = std::move(rom);
    }

    std::vector<MemoryExpectation> result;
};

struct TwoPlayersFramebufferRunnerParams : BaseRunnerParams {
    TwoPlayersFramebufferRunnerParams(std::string&& rom, std::string&& expected, std::string&& rom2,
                                      std::string&& expected2) :
        result {std::move(expected)},
        rom2 {std::move(rom2)},
        result2 {std::move(expected2)} {
        this->rom = std::move(rom);
    }

    std::string result;
    std::string rom2;
    std::string result2;

    const Appearance* appearance {};
    ColorTolerance color_tolerance {};
};

struct RunnerAdapter {
    using Params = std::variant<FramebufferRunnerParams, SerialRunnerParams, MemoryRunnerParams,
                                TwoPlayersFramebufferRunnerParams>;

    RunnerAdapter(std::string roms_prefix = "", std::string results_prefix = "") :
        roms_prefix {std::move(roms_prefix)},
        results_prefix {std::move(results_prefix)} {
    }

    bool run(const Params& p) {
        if (std::holds_alternative<FramebufferRunnerParams>(p)) {
            const auto& pf = std::get<FramebufferRunnerParams>(p);
            return FramebufferRunner()
                .rom(roms_prefix + pf.rom)
                .expect_framebuffer(results_prefix + pf.result)
                .appearance(pf.appearance)
                .color_tolerance(pf.color_tolerance.red, pf.color_tolerance.green, pf.color_tolerance.blue)
                .check_interval_ticks(pf.check_interval_ticks)
                .max_ticks(pf.max_ticks)
                .stop_at_instruction(pf.stop_at_instruction)
                .force_check(pf.force_check)
                .limit_speed(pf.limit_speed)
                .schedule_inputs(pf.inputs)
                .run();
        }

        if (std::holds_alternative<SerialRunnerParams>(p)) {
            const auto& ps = std::get<SerialRunnerParams>(p);

            return SerialRunner()
                .rom(roms_prefix + ps.rom)
                .expect_output(ps.result)
                .check_interval_ticks(ps.check_interval_ticks)
                .max_ticks(ps.max_ticks)
                .stop_at_instruction(ps.stop_at_instruction)
                .force_check(ps.force_check)
                .limit_speed(ps.limit_speed)
                .schedule_inputs(ps.inputs)
                .run();
        }

        if (std::holds_alternative<MemoryRunnerParams>(p)) {
            const auto& pm = std::get<MemoryRunnerParams>(p);

            return MemoryRunner()
                .rom(roms_prefix + pm.rom)
                .expect_output(pm.result)
                .check_interval_ticks(pm.check_interval_ticks)
                .max_ticks(pm.max_ticks)
                .stop_at_instruction(pm.stop_at_instruction)
                .force_check(pm.force_check)
                .limit_speed(pm.limit_speed)
                .schedule_inputs(pm.inputs)
                .run();
        }

        if (std::holds_alternative<TwoPlayersFramebufferRunnerParams>(p)) {
            const auto& pf2p = std::get<TwoPlayersFramebufferRunnerParams>(p);

            return TwoPlayersFramebufferRunner()
                .rom(roms_prefix + pf2p.rom)
                .expect_framebuffer1(results_prefix + pf2p.result)
                .rom2(roms_prefix + pf2p.rom2)
                .expect_framebuffer2(results_prefix + pf2p.result2)
                .appearance(pf2p.appearance)
                .color_tolerance(pf2p.color_tolerance.red, pf2p.color_tolerance.green, pf2p.color_tolerance.blue)
                .check_interval_ticks(pf2p.check_interval_ticks)
                .max_ticks(pf2p.max_ticks)
                .stop_at_instruction(pf2p.stop_at_instruction)
                .force_check(pf2p.force_check)
                .limit_speed(pf2p.limit_speed)
                .schedule_inputs(pf2p.inputs)
                .run();
        }

        ASSERT_NO_ENTRY();
        return false;
    }

private:
    std::string roms_prefix;
    std::string results_prefix;
};

#endif // RUNNERS_H