#include "runtests.h"

#include <deque>

#include "simdjson.h"

#include "catch2/catch_test_macros.hpp"

namespace {
Joypad::KeyState parse_joypad_key_state(const std::string_view state) {
    if (state == "pressed") {
        return Joypad::KeyState::Pressed;
    }
    if (state == "released") {
        return Joypad::KeyState::Released;
    }

    ASSERT_NO_ENTRY();
    return {};
}

Joypad::Key parse_joypad_key(const std::string_view key) {
    if (key == "down") {
        return Joypad::Key::Down;
    }
    if (key == "up") {
        return Joypad::Key::Up;
    }
    if (key == "left") {
        return Joypad::Key::Left;
    }
    if (key == "right") {
        return Joypad::Key::Right;
    }
    if (key == "start") {
        return Joypad::Key::Start;
    }
    if (key == "select") {
        return Joypad::Key::Select;
    }
    if (key == "b") {
        return Joypad::Key::B;
    }
    if (key == "a") {
        return Joypad::Key::A;
    }

    ASSERT_NO_ENTRY();
    return {};
}

std::optional<RunnerAdapter::Params> json_to_runner_params(simdjson::ondemand::object&& object) {
    if (auto enabled_element = object["enabled"]; enabled_element.error() == simdjson::SUCCESS) {
        if (!enabled_element.get_bool()) {
            // Disabled test
            return std::nullopt;
        }
    }

    std::string rom {object["rom"].get_string().value()};

    auto parse_base_params = [](auto object, auto& params) {
        params.rom = object["rom"].get_string().value();

        if (auto check_interval_ticks_element = object["check_interval_ticks"];
            check_interval_ticks_element.error() == simdjson::SUCCESS) {
            params.check_interval_ticks = check_interval_ticks_element.get_uint64();
        }

        if (auto max_ticks_element = object["max_ticks"]; max_ticks_element.error() == simdjson::SUCCESS) {
            params.max_ticks = max_ticks_element.get_uint64();
        }

        if (auto stop_at_instruction_element = object["stop_at_instruction"];
            stop_at_instruction_element.error() == simdjson::SUCCESS) {
            params.stop_at_instruction = stop_at_instruction_element.get_uint64();
        }

        if (auto force_check_element = object["force_check"]; force_check_element.error() == simdjson::SUCCESS) {
            params.force_check = force_check_element.get_bool();
        }

        if (auto inputs_elements = object["inputs"]; inputs_elements.error() == simdjson::SUCCESS) {
            std::vector<JoypadInput> inputs {};

            auto inputs_array = inputs_elements.get_array();
            for (auto input_element : inputs_array) {
                auto input_object = input_element.get_object();

                JoypadInput input {};
                input.tick = input_object["tick"].get_uint64();
                input.key = parse_joypad_key(input_object["key"].get_string());
                input.state = parse_joypad_key_state(input_object["state"].get_string());

                inputs.push_back(input);
            }

            params.inputs = std::move(inputs);
        }
    };

    auto parse_framebuffer_params = [](auto object, auto& params) {
        if (auto color_tolerance_element = object["color_tolerance"];
            color_tolerance_element.error() == simdjson::SUCCESS) {
            auto color_tolerance_object = color_tolerance_element.get_object();
            params.color_tolerance.red = color_tolerance_object["red"].get_uint64();
            params.color_tolerance.green = color_tolerance_object["green"].get_uint64();
            params.color_tolerance.blue = color_tolerance_object["blue"].get_uint64();
        }

#ifndef ENABLE_CGB
        if (auto palette_element = object["palette"]; palette_element.error() == simdjson::SUCCESS) {
            auto palette = palette_element.get_string().value();
            if (palette == "grey") {
                params.appearance = &GREY_PALETTE;
            }
        }
#endif
    };

    if (auto framebuffer_element = object["framebuffer"]; framebuffer_element.error() == simdjson::SUCCESS) {
        // FramebufferRunnerParams or TwoPlayersFramebufferRunnerParams
        std::string framebuffer_result {framebuffer_element.get_string().value()};

        if (auto rom2_element = object["rom2"]; rom2_element.error() == simdjson::SUCCESS) {
            // TwoPlayersFramebufferRunnerParams

            std::string rom2 {rom2_element.get_string().value()};
            std::string framebuffer_result2 {object["framebuffer2"].get_string().value()};

            TwoPlayersFramebufferRunnerParams params {std::move(rom), std::move(framebuffer_result), std::move(rom2),
                                                      std::move(framebuffer_result2)};
            parse_base_params(object, params);
            parse_framebuffer_params(object, params);

            return params;
        }

        // FramebufferRunnerParams

        FramebufferRunnerParams params {std::move(rom), std::move(framebuffer_result)};

        parse_base_params(object, params);
        parse_framebuffer_params(object, params);

        return params;
    }

    if (auto serial_element = object["serial"]; serial_element.error() == simdjson::SUCCESS) {
        // SerialRunnerParams

        std::vector<uint8_t> serial_result {};
        for (auto serial_byte_element : serial_element.get_array()) {
            serial_result.push_back(serial_byte_element.get_uint64().value());
        }

        SerialRunnerParams params {std::move(rom), std::move(serial_result)};

        parse_base_params(object, params);

        return params;
    }

    if (auto memory_element = object["memory"]; memory_element.error() == simdjson::SUCCESS) {
        // MemoryRunnerParams

        std::vector<MemoryExpectation> memory_result {};
        for (auto memory_result_element : memory_element.get_array()) {
            auto memory_result_object = memory_result_element.get_object();
            MemoryExpectation expected {};
            expected.address = memory_result_object["address"].get_uint64();
            expected.success_value = memory_result_object["value"].get_uint64();
            if (auto fail_value_element = memory_result_object["fail_value"];
                fail_value_element.error() == simdjson::SUCCESS) {
                expected.fail_value = fail_value_element.get_uint64();
            }

            memory_result.push_back(expected);
        }

        MemoryRunnerParams params {std::move(rom), std::move(memory_result)};

        parse_base_params(object, params);

        return params;
    }

    ASSERT_NO_ENTRY();
    return std::nullopt;
}

struct Section {
    std::string name;
    std::vector<RunnerAdapter::Params> tests;
};

const std::vector<Section>& build_or_get_test_roms_sections_from_json(const std::string& json_path) {
    static std::unordered_map<std::string, std::vector<Section>> json_cache;

    if (json_cache.find(json_path) == json_cache.end()) {
        std::vector<Section> sections;

        simdjson::ondemand::parser parser;
        simdjson::padded_string json = simdjson::padded_string::load(json_path);
        simdjson::ondemand::document doc = parser.iterate(json);
        simdjson::ondemand::object root = doc.get_object();

        for (auto section : root) {
            auto section_key = section.unescaped_key().value();
            auto section_value = section.value();

            std::string section_name {section_key};

            std::vector<RunnerAdapter::Params> all_params {};
            for (auto test_rom : section_value) {
                if (const auto params = json_to_runner_params(test_rom.get_object()); params.has_value()) {
                    all_params.push_back(*params);
                }
            }

            sections.push_back(Section {std::move(section_name), std::move(all_params)});
        }

        json_cache[json_path] = std::move(sections);
    }

    return json_cache[json_path];
}
} // namespace

void run_test_roms_from_params(const std::string& roms_folder, const std::string& results_folder,
                               const std::vector<RunnerAdapter::Params>& all_params) {
    RunnerAdapter adapter {roms_folder, results_folder};
    for (const auto& params : all_params) {
        REQUIRE(adapter.run(params));
    }
}

void run_test_roms_from_json(const std::string& roms_folder, const std::string& results_folder,
                             const std::string& json_path, const std::string& section_filter) {
    // Since Catch will re-enter this branch multiple times, for each section, parse and cache the json only once.
    const std::vector<Section>& sections = build_or_get_test_roms_sections_from_json(json_path);

    RunnerAdapter adapter {roms_folder, results_folder};

    for (const auto& [name, tests_params] : sections) {
        if (!section_filter.empty() && !starts_with(name, section_filter)) {
            // If a filter is given, keep only the sections having the filter as prefix.
            continue;
        }

        SECTION(name) {
            for (const auto& test_params : tests_params) {
                REQUIRE(adapter.run(test_params));
            }
        }
    }
}

void run_framebuffer_test_roms_from_folder(const std::string& roms_folder, const std::string& result) {
    RunnerAdapter adapter {};
    SECTION(roms_folder) {
        for (const auto& entry : iterate_directory(roms_folder)) {
            if (entry.type == DirectoryIteratorEntry::FileType::File) {
                REQUIRE(adapter.run(FramebufferRunnerParams {entry.path.c_str(), std::string {result}}));
            }
        }
    }
}

void run_framebuffer_test_roms_from_folder_recursive(const std::string& roms_folder, const std::string& result) {
    RunnerAdapter adapter {};
    SECTION(roms_folder) {
        for (const auto& entry : recursive_iterate_directory(roms_folder)) {
            if (entry.type == DirectoryIteratorEntry::FileType::File) {
                REQUIRE(adapter.run(FramebufferRunnerParams {entry.path.c_str(), std::string {result}}));
            }
        }
    }
}

void run_memory_test_roms_from_folder(const std::string& roms_folder, const MemoryExpectation& expectation) {
    RunnerAdapter adapter {};
    SECTION(roms_folder) {
        for (const auto& entry : iterate_directory(roms_folder)) {
            if (entry.type == DirectoryIteratorEntry::FileType::File) {
                REQUIRE(adapter.run(MemoryRunnerParams {entry.path.c_str(), {expectation}}));
            }
        }
    }
}

void run_memory_test_roms_from_folder_recursive(const std::string& roms_folder, const MemoryExpectation& expectation) {
    RunnerAdapter adapter {};
    SECTION(roms_folder) {
        for (const auto& entry : recursive_iterate_directory(roms_folder)) {
            if (entry.type == DirectoryIteratorEntry::FileType::File) {
                REQUIRE(adapter.run(MemoryRunnerParams {entry.path.c_str(), {expectation}}));
            }
        }
    }
}