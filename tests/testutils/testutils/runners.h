#ifndef RUNNERS_H
#define RUNNERS_H

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

struct ColorTolerance {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};
struct CheckIntervalTicks {
    uint64_t value;
};
struct MaxTicks {
    uint64_t value;
};
struct StopAtInstruction {
    uint8_t instruction;
};
struct ForceCheck {};

#ifdef ENABLE_CGB
constexpr ColorTolerance COLOR_TOLERANCE_LOW = {5, 5, 5};
constexpr ColorTolerance COLOR_TOLERANCE_MEDIUM = {10, 10, 10};
constexpr ColorTolerance COLOR_TOLERANCE_INSANE = {150, 150, 150};
#endif

#ifndef ENABLE_CGB
constexpr Appearance GREY_PALETTE {0xFFFF, {0xFFFF, 0xAD55, 0x52AA, 0x0000}};
#endif

#ifdef ENABLE_BOOTROM
extern std::string boot_rom;
#endif

#ifdef ENABLE_BOOTROM
constexpr uint64_t BOOT_DURATION = 23'440'328;
#else
constexpr uint64_t BOOT_DURATION = 0;
#endif

constexpr uint64_t DURATION_VERY_LONG = BOOT_DURATION + 250'000'000;
constexpr uint64_t DURATION_LONG = BOOT_DURATION + 100'000'000;
constexpr uint64_t DURATION_MEDIUM = BOOT_DURATION + 30'000'000;
constexpr uint64_t DURATION_SHORT = BOOT_DURATION + 5'000'000;
constexpr uint64_t DURATION_VERY_SHORT = BOOT_DURATION + 1'500'000;
constexpr uint64_t DEFAULT_DURATION = DURATION_LONG;

inline std::function<void(const std::string&)> make_default_runner_log() {
    return [](const std::string& message) {
        std::cerr << message << std::endl << std::flush;
    };
}

inline std::function<void(const std::string&)> runner_log = make_default_runner_log();

template <typename RunnerImpl>
class Runner {
public:
    struct JoypadInput {
        uint64_t ticks {};
        Joypad::KeyState state {};
        Joypad::Key key {};
    };

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

    RunnerImpl& schedule_inputs(std::vector<JoypadInput> inputs) {
        inputs_ = std::move(inputs);
        // Eventually at BOOT_DURATION time to inputs timing
        for (auto& in : inputs_) {
            in.ticks += BOOT_DURATION;
        }
        return static_cast<RunnerImpl&>(*this);
    }

    bool run() {
        can_run = true;

        auto* impl = static_cast<RunnerImpl*>(this);
        impl->on_run();

        bool has_ever_checked {false};

        for (tick = core.ticks; tick <= max_ticks_ && can_run; tick += 4) {
            // Eventually submit scheduled Joypad input.
            if (!inputs_.empty()) {
                for (auto it = inputs_.begin(); it != inputs_.end(); it++) {
                    if (tick >= it->ticks) {
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
                throw std::runtime_error("=== " + rom_name + " ===\n" + err.what());
            }

            impl->on_cycle();

            // Check expectation.
            if (impl->should_check_expectation()) {
                has_ever_checked = true;
                if (impl->check_expectation()) {
                    return true;
                }
            }
        }

        if (has_ever_checked) {
            impl->on_expectation_failed();
        } else if (impl->should_ever_check_expectation()) {
            runner_log("=== " + rom_name + " ===");
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

    bool check_expectation() {
        return true;
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

    bool check_expectation() {
        memcpy(last_framebuffer, gb->lcd.get_pixels(), FRAMEBUFFER_SIZE);
        if (color_tolerance_.red == 0 && color_tolerance_.green == 0 && color_tolerance_.blue == 0) {
            return are_framebuffer_equals(last_framebuffer, expected_framebuffer);
        } else {
            return are_framebuffer_equals_with_tolerance(last_framebuffer, expected_framebuffer, color_tolerance_.red,
                                                         color_tolerance_.green, color_tolerance_.blue);
        }
    }

    void on_expectation_failed() {
        // Framebuffer not equals: figure out where's the (first) problem
        std::stringstream output_message;

        output_message << "=== " << rom_name << " ===" << std::endl;

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

    bool check_expectation() {
        last_output = serial_buffer.buffer;
        return serial_buffer.buffer == expected_output;
    }

    void on_expectation_failed() {
        runner_log("=== " + rom_name + " ===");
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

    MemoryRunner& expect_output(const std::vector<std::pair<uint16_t, uint8_t>>& output) {
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

    bool check_expectation() {
        bool match = true;

        last_output.clear();

        for (const auto& [address, value] : expected_output) {
            uint8_t real_value = gb->mmu.bus_accessors[Device::Cpu][address].read_bus(address);
            match = match && (real_value == value);
            last_output.emplace_back(address, real_value);
        }

        return match;
    }

    void on_expectation_failed() {
        runner_log("=== " + rom_name + " ===");

        std::string expected_output_str = "[" +
                                          join(expected_output, ",",
                                               [](const std::pair<uint16_t, uint8_t>& address_to_value) {
                                                   std::stringstream str;
                                                   str << "{" << hex(address_to_value.first) << ", "
                                                       << hex(address_to_value.second) << "}";
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
    std::vector<std::pair<uint16_t, uint8_t>> expected_output;
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

    bool check_expectation() {
        memcpy(last_framebuffer1, gb->lcd.get_pixels(), FRAMEBUFFER_SIZE);
        memcpy(last_framebuffer2, gb2->lcd.get_pixels(), FRAMEBUFFER_SIZE);
        if (color_tolerance_.red == 0 && color_tolerance_.green == 0 && color_tolerance_.blue == 0) {
            return are_framebuffer_equals(last_framebuffer1, expected_framebuffer1) &&
                   are_framebuffer_equals(last_framebuffer2, expected_framebuffer2);
        } else {
            return are_framebuffer_equals_with_tolerance(last_framebuffer1, expected_framebuffer1, color_tolerance_.red,
                                                         color_tolerance_.green, color_tolerance_.blue) &&
                   are_framebuffer_equals_with_tolerance(last_framebuffer1, expected_framebuffer2, color_tolerance_.red,
                                                         color_tolerance_.green, color_tolerance_.blue);
        }
    }

    void on_expectation_failed() {
        // Framebuffer not equals: figure out where's the (first) problem
        std::stringstream output_message;

        output_message << "=== " << rom_name << "," << rom_name2 << "===" << std::endl;

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

using Inputs = std::vector<FramebufferRunner::JoypadInput>;

struct FramebufferRunnerParams {
    using Param = std::variant<std::monostate,
#ifndef ENABLE_CGB
                               const Appearance*,
#endif
                               ColorTolerance, CheckIntervalTicks, MaxTicks, StopAtInstruction, ForceCheck,
                               std::vector<FramebufferRunner::JoypadInput>>;

    FramebufferRunnerParams(std::string&& rom, std::string&& expected, Param param1 = std::monostate {},
                            Param param2 = std::monostate {}) :
        rom(std::move(rom)),
        result {std::move(expected)} {

        auto parseParam = [this](Param& param) {
#ifndef ENABLE_CGB
            if (std::holds_alternative<const Appearance*>(param)) {
                appearance = std::get<const Appearance*>(param);
            } else
#endif
                if (std::holds_alternative<ColorTolerance>(param)) {
                color_tolerance = std::get<ColorTolerance>(param);
            } else if (std::holds_alternative<CheckIntervalTicks>(param)) {
                check_interval_ticks = std::get<CheckIntervalTicks>(param).value;
            } else if (std::holds_alternative<MaxTicks>(param)) {
                max_ticks = std::get<MaxTicks>(param).value;
            } else if (std::holds_alternative<StopAtInstruction>(param)) {
                stop_at_instruction = std::get<StopAtInstruction>(param).instruction;
            } else if (std::holds_alternative<ForceCheck>(param)) {
                force_check = true;
            } else if (std::holds_alternative<std::vector<FramebufferRunner::JoypadInput>>(param)) {
                inputs = std::get<std::vector<FramebufferRunner::JoypadInput>>(param);
            }
        };

        parseParam(param1);
        parseParam(param2);
    }

    std::string rom;
    std::string result;
    const Appearance* appearance {};
    uint64_t check_interval_ticks {DURATION_VERY_SHORT};
    uint64_t max_ticks {DEFAULT_DURATION};
    std::optional<uint8_t> stop_at_instruction {};
    bool force_check {};
    std::vector<FramebufferRunner::JoypadInput> inputs {};
    ColorTolerance color_tolerance {};
};

struct SerialRunnerParams {
    SerialRunnerParams(std::string&& rom, std::vector<uint8_t>&& expected, uint64_t maxTicks_ = DURATION_MEDIUM) :
        rom(std::move(rom)),
        result {std::move(expected)},
        max_ticks {maxTicks_} {
    }

    std::string rom;
    std::vector<uint8_t> result;
    uint64_t max_ticks;
};

struct MemoryRunnerParams {
    MemoryRunnerParams(std::string&& rom, std::vector<std::pair<uint16_t, uint8_t>>&& expected,
                       uint64_t maxTicks_ = DURATION_MEDIUM) :
        rom(std::move(rom)),
        result {std::move(expected)},
        max_ticks {maxTicks_} {
    }

    std::string rom;
    std::vector<std::pair<uint16_t, uint8_t>> result;
    uint64_t max_ticks;
};

struct TwoPlayersFramebufferRunnerParams {
    using Param = std::variant<std::monostate,
#ifndef ENABLE_CGB
                               const Appearance*,
#endif
                               ColorTolerance, MaxTicks>;

    TwoPlayersFramebufferRunnerParams(std::string&& rom, std::string&& expected, std::string&& rom2,
                                      std::string&& expected2, Param param1 = std::monostate {},
                                      Param param2 = std::monostate {}) :
        rom(std::move(rom)),
        result {std::move(expected)},
        rom2 {std::move(rom2)},
        result2 {std::move(expected2)} {

        auto parseParam = [this](Param& param) {
#ifndef ENABLE_CGB
            if (std::holds_alternative<const Appearance*>(param)) {
                appearance = std::get<const Appearance*>(param);
            } else
#endif
                if (std::holds_alternative<ColorTolerance>(param)) {
                color_tolerance = std::get<ColorTolerance>(param);
            } else if (std::holds_alternative<MaxTicks>(param)) {
                max_ticks = std::get<MaxTicks>(param).value;
            }
        };

        parseParam(param1);
        parseParam(param2);
    }

    std::string rom;
    std::string result;
    std::string rom2;
    std::string result2;
    const Appearance* appearance {};
    uint64_t max_ticks {DEFAULT_DURATION};
    ColorTolerance color_tolerance {};
};

struct RunnerAdapter {
    using Params = std::variant<FramebufferRunnerParams, SerialRunnerParams, MemoryRunnerParams,
                                TwoPlayersFramebufferRunnerParams>;

    RunnerAdapter(std::string roms_prefix, std::string results_prefix) :
        roms_prefix {std::move(roms_prefix)},
        results_prefix {std::move(results_prefix)} {
    }

    bool run(const Params& p) {
        if (std::holds_alternative<FramebufferRunnerParams>(p)) {
            const auto& pf = std::get<FramebufferRunnerParams>(p);
            return FramebufferRunner()
                .rom(roms_prefix + pf.rom)
                .appearance(pf.appearance)
                .max_ticks(pf.max_ticks)
                .check_interval_ticks(pf.check_interval_ticks)
                .stop_at_instruction(pf.stop_at_instruction)
                .expect_framebuffer(results_prefix + pf.result)
                .color_tolerance(pf.color_tolerance.red, pf.color_tolerance.green, pf.color_tolerance.blue)
                .force_check(pf.force_check)
                .schedule_inputs(pf.inputs)
                .run();
        }

        if (std::holds_alternative<SerialRunnerParams>(p)) {
            const auto& ps = std::get<SerialRunnerParams>(p);

            return SerialRunner()
                .rom(roms_prefix + ps.rom)
                .max_ticks(ps.max_ticks)
                .check_interval_ticks(DURATION_VERY_SHORT)
                .expect_output(ps.result)
                .run();
        }

        if (std::holds_alternative<MemoryRunnerParams>(p)) {
            const auto& pm = std::get<MemoryRunnerParams>(p);

            return MemoryRunner()
                .rom(roms_prefix + pm.rom)
                .max_ticks(pm.max_ticks)
                .check_interval_ticks(DURATION_VERY_SHORT)
                .expect_output(pm.result)
                .run();
        }

        if (std::holds_alternative<TwoPlayersFramebufferRunnerParams>(p)) {
            const auto& pf2p = std::get<TwoPlayersFramebufferRunnerParams>(p);

            return TwoPlayersFramebufferRunner()
                .rom(roms_prefix + pf2p.rom)
                .rom2(roms_prefix + pf2p.rom2)
                .appearance(pf2p.appearance)
                .max_ticks(pf2p.max_ticks)
                .check_interval_ticks(DURATION_VERY_SHORT)
                .expect_framebuffer1(results_prefix + pf2p.result)
                .expect_framebuffer2(results_prefix + pf2p.result2)
                .color_tolerance(pf2p.color_tolerance.red, pf2p.color_tolerance.green, pf2p.color_tolerance.blue)
                .run();
        }

        ASSERT_NO_ENTRY();
        return false;
    }

private:
    std::string roms_prefix;
    std::string results_prefix;
};

using F = FramebufferRunnerParams;
using S = SerialRunnerParams;
using M = MemoryRunnerParams;

using F2P = TwoPlayersFramebufferRunnerParams;

#endif // RUNNERS_H