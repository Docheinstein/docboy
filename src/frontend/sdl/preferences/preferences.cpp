#include "preferences/preferences.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "utils/formatters.h"
#include "utils/path.h"
#include "utils/strings.h"

#include "extra/ini/reader/reader.h"
#include "extra/ini/writer/writer.h"

namespace {
template <typename T, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()>
std::optional<T> parse_integer(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    auto val = std::strtoll(cstr, &endptr, 10);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    return std::clamp(val, static_cast<decltype(val)>(min), static_cast<decltype(val)>(max));
}

template <uint32_t min_num, uint32_t min_denum, uint32_t max_num, uint32_t max_denum>
std::optional<double> parse_double(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    auto val = std::strtod(cstr, &endptr);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    static constexpr double min = min_denum ? (min_num / min_denum) : 0;
    static constexpr double max = max_denum ? (max_num / max_denum) : 0;

    return std::clamp(val, static_cast<decltype(val)>(min), static_cast<decltype(val)>(max));
}

std::optional<bool> parse_bool(const std::string& s) {
    return parse_integer<bool>(s);
}

std::optional<uint16_t> parse_hex_uint16(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    uint16_t val = std::strtol(cstr, &endptr, 16);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    return val;
}

std::optional<SDL_Keycode> parse_keycode(const std::string& s) {
    SDL_Keycode keycode = SDL_GetKeyFromName(s.c_str());
    return keycode != SDLK_UNKNOWN ? std::optional {keycode} : std::nullopt;
}

template <uint8_t Size>
std::optional<std::array<uint16_t, Size>> parse_hex_array(const std::string& s) {
    std::vector<std::string> tokens;
    split(s, std::back_inserter(tokens), [](char ch) {
        return ch == ',';
    });

    if (tokens.size() != Size) {
        return std::nullopt;
    }

    std::array<uint16_t, Size> ret {};

    for (uint32_t i = 0; i < Size; i++) {
        if (auto u = parse_hex_uint16(tokens[i])) {
            ret[i] = *u;
        } else {
            return std::nullopt;
        }
    }

    return ret;
}
} // namespace

std::string get_preferences_path() {
    const char* pref_path_cstr = SDL_GetPrefPath("", "DocBoy");
    std::string pref_path = (Path {pref_path_cstr} / "prefs.ini").string();
    return pref_path;
}

void read_preferences(const std::string& path, Preferences& p) {
    IniReader ini_reader;
    ini_reader.add_comment_prefix("#");
#ifndef ENABLE_CGB
    ini_reader.add_property("dmg_palette", p.dmg_palette,
                            [](const std::string& s) -> std::optional<UiController::LcdAppearance> {
                                auto a = parse_hex_array<5>(s);
                                if (!a) {
                                    return std::nullopt;
                                }

                                UiController::LcdAppearance appearance {};
                                appearance.default_color = (*a)[4];
                                std::copy_n(a->begin(), 4, appearance.palette.begin());
                                return appearance;
                            });
#endif
    ini_reader.add_property("a", p.keys.player1.a, parse_keycode);
    ini_reader.add_property("b", p.keys.player1.b, parse_keycode);
    ini_reader.add_property("start", p.keys.player1.start, parse_keycode);
    ini_reader.add_property("select", p.keys.player1.select, parse_keycode);
    ini_reader.add_property("left", p.keys.player1.left, parse_keycode);
    ini_reader.add_property("up", p.keys.player1.up, parse_keycode);
    ini_reader.add_property("right", p.keys.player1.right, parse_keycode);
    ini_reader.add_property("down", p.keys.player1.down, parse_keycode);
#ifdef ENABLE_TWO_PLAYERS_MODE
    ini_reader.add_property("p2_a", p.keys.player2.a, parse_keycode);
    ini_reader.add_property("p2_b", p.keys.player2.b, parse_keycode);
    ini_reader.add_property("p2_start", p.keys.player2.start, parse_keycode);
    ini_reader.add_property("p2_select", p.keys.player2.select, parse_keycode);
    ini_reader.add_property("p2_left", p.keys.player2.left, parse_keycode);
    ini_reader.add_property("p2_up", p.keys.player2.up, parse_keycode);
    ini_reader.add_property("p2_right", p.keys.player2.right, parse_keycode);
    ini_reader.add_property("p2_down", p.keys.player2.down, parse_keycode);
#endif
    ini_reader.add_property("scaling", p.scaling, parse_integer<uint32_t>);
    ini_reader.add_property("x", p.x, parse_integer<int32_t>);
    ini_reader.add_property("y", p.y, parse_integer<int32_t>);

#ifdef ENABLE_AUDIO
    ini_reader.add_property("audio", p.audio, parse_bool);
#ifdef ENABLE_TWO_PLAYERS_MODE
    ini_reader.add_property("audio_player_source", p.audio_player_source, parse_integer<uint8_t>);
#endif
    ini_reader.add_property("volume", p.volume, parse_integer<uint8_t, 0, 100>);
    ini_reader.add_property("high_pass_filter", p.high_pass_filter, parse_bool);
    ini_reader.add_property("dynamic_sample_rate_control", p.dynamic_sample_rate.enabled, parse_bool);
    ini_reader.add_property("dynamic_sample_rate_control_max_latency", p.dynamic_sample_rate.max_latency,
                            parse_integer<uint32_t>);
    ini_reader.add_property("dynamic_sample_rate_control_moving_average_factor",
                            p.dynamic_sample_rate.moving_average_factor, parse_double<0, 0, 1, 1>);
    ini_reader.add_property("dynamic_sample_rate_control_max_pitch_slack_factor",
                            p.dynamic_sample_rate.max_pitch_slack_factor, parse_double<0, 0, 1, 1>);
#endif

#ifdef ENABLE_TWO_PLAYERS_MODE
    ini_reader.add_property("serial_link", p.serial_link, parse_bool);
#endif

    const auto result = ini_reader.parse(path);
    switch (result.outcome) {
    case IniReader::Result::Outcome::Success:
        break;
    case IniReader::Result::Outcome::ErrorReadFailed:
        std::cerr << "ERROR: failed to read '" << path << "'" << std::endl;
        exit(2);
    case IniReader::Result::Outcome::ErrorParseFailed:
        std::cerr << "ERROR: failed to parse  '" << path << "': error at line " << result.last_read_line << std::endl;
        exit(2);
    }
}
void write_preferences(const std::string& path, const Preferences& p) {
    std::map<std::string, std::string> properties;

#ifndef ENABLE_CGB
    properties.emplace("dmg_palette", join(p.dmg_palette.palette, ",",
                                           [](uint16_t val) {
                                               return hex(val);
                                           }) +
                                          "," + hex(p.dmg_palette.default_color));
#endif
    properties.emplace("a", SDL_GetKeyName(p.keys.player1.a));
    properties.emplace("b", SDL_GetKeyName(p.keys.player1.b));
    properties.emplace("start", SDL_GetKeyName(p.keys.player1.start));
    properties.emplace("select", SDL_GetKeyName(p.keys.player1.select));
    properties.emplace("left", SDL_GetKeyName(p.keys.player1.left));
    properties.emplace("up", SDL_GetKeyName(p.keys.player1.up));
    properties.emplace("right", SDL_GetKeyName(p.keys.player1.right));
    properties.emplace("down", SDL_GetKeyName(p.keys.player1.down));
#ifdef ENABLE_TWO_PLAYERS_MODE
    properties.emplace("p2_a", SDL_GetKeyName(p.keys.player2.a));
    properties.emplace("p2_b", SDL_GetKeyName(p.keys.player2.b));
    properties.emplace("p2_start", SDL_GetKeyName(p.keys.player2.start));
    properties.emplace("p2_select", SDL_GetKeyName(p.keys.player2.select));
    properties.emplace("p2_left", SDL_GetKeyName(p.keys.player2.left));
    properties.emplace("p2_up", SDL_GetKeyName(p.keys.player2.up));
    properties.emplace("p2_right", SDL_GetKeyName(p.keys.player2.right));
    properties.emplace("p2_down", SDL_GetKeyName(p.keys.player2.down));
#endif
    properties.emplace("scaling", std::to_string(p.scaling));
    properties.emplace("x", std::to_string(p.x));
    properties.emplace("y", std::to_string(p.y));

#ifdef ENABLE_AUDIO
    properties.emplace("audio", std::to_string(p.audio));
#ifdef ENABLE_TWO_PLAYERS_MODE
    properties.emplace("audio_player_source", std::to_string(p.audio_player_source));
#endif
    properties.emplace("volume", std::to_string(p.volume));
    properties.emplace("high_pass_filter", std::to_string(p.high_pass_filter));
    properties.emplace("dynamic_sample_rate_control", std::to_string(p.dynamic_sample_rate.enabled));
    properties.emplace("dynamic_sample_rate_control_max_latency", std::to_string(p.dynamic_sample_rate.max_latency));
    properties.emplace("dynamic_sample_rate_control_moving_average_factor",
                       std::to_string(p.dynamic_sample_rate.moving_average_factor));
    properties.emplace("dynamic_sample_rate_control_max_pitch_slack_factor",
                       std::to_string(p.dynamic_sample_rate.max_pitch_slack_factor));
#endif

#ifdef ENABLE_TWO_PLAYERS_MODE
    properties.emplace("serial_link", std::to_string(p.serial_link));
#endif

    IniWriter ini_writer;
    if (!ini_writer.write(properties, path)) {
        std::cerr << "WARN: failed to write '" << path << "'" << std::endl;
    }
}