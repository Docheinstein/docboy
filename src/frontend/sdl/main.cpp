#include <chrono>
#include <iostream>
#include <map>

#include "SDL3/SDL.h"

#include "docboy/cartridge/factory.h"

#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/uicontroller.h"
#include "screens/gamescreen.h"
#include "screens/launcherscreen.h"
#include "windows/window.h"

#include "extra/cartridge/header.h"
#include "extra/img/imgmanip.h"
#include "extra/ini/reader/reader.h"
#include "extra/ini/writer/writer.h"
#include "extra/serial/endpoints/console.h"

#include "args/args.h"

#include "utils/formatters.h"
#include "utils/os.h"
#include "utils/strings.h"

#ifdef NFD
#include "nfd.h"
#endif

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/factory.h"
#endif

#ifdef ENABLE_DEBUGGER
#include "controllers/debuggercontroller.h"
#include "docboy/debugger/backend.h"
#include "docboy/debugger/frontend.h"
#endif

namespace {

struct Preferences {
    Lcd::Palette palette {};
    struct {
        SDL_Keycode a {};
        SDL_Keycode b {};
        SDL_Keycode start {};
        SDL_Keycode select {};
        SDL_Keycode left {};
        SDL_Keycode up {};
        SDL_Keycode right {};
        SDL_Keycode down {};
    } keys {};
    uint32_t scaling {};
    int x {};
    int y {};
#ifdef ENABLE_AUDIO
    bool audio {};
    uint8_t volume {};
    struct {
        bool enabled {};
        uint32_t max_latency {};
        double moving_average_factor {};
        double max_pitch_slack_factor {};
    } dynamic_sample_rate {};
#endif
};

Preferences make_default_preferences() {
    Preferences prefs {};
    prefs.palette = Lcd::DEFAULT_PALETTE;
    prefs.keys.a = SDLK_Z;
    prefs.keys.b = SDLK_X;
    prefs.keys.start = SDLK_RETURN;
    prefs.keys.select = SDLK_TAB;
    prefs.keys.left = SDLK_LEFT;
    prefs.keys.up = SDLK_UP;
    prefs.keys.right = SDLK_RIGHT;
    prefs.keys.down = SDLK_DOWN;
    prefs.scaling = 2;
    prefs.x = 200;
    prefs.y = 200;
#ifdef ENABLE_AUDIO
    prefs.audio = true;
    prefs.volume = 100;
    prefs.dynamic_sample_rate.enabled = true;
    prefs.dynamic_sample_rate.max_latency = 50;
    prefs.dynamic_sample_rate.moving_average_factor = 0.005;
    prefs.dynamic_sample_rate.max_pitch_slack_factor = 0.005;
#endif
    return prefs;
}

void dump_cartridge_info(const ICartridge& cartridge) {
    const auto header = CartridgeHeader::parse(cartridge);
    std::cout << "Title             :  " << header.title_as_string() << "\n";
    std::cout << "Cartridge type    :  " << hex(header.cartridge_type) << "     ("
              << header.cartridge_type_description() << ")\n";
    std::cout << "Licensee (new)    :  " << hex(header.new_licensee_code) << "  ("
              << header.new_licensee_code_description() << ")\n";
    std::cout << "Licensee (old)    :  " << hex(header.old_licensee_code) << "     ("
              << header.old_licensee_code_description() << ")\n";
    std::cout << "ROM Size          :  " << hex(header.rom_size) << "     (" << header.rom_size_description() << ")\n";
    std::cout << "RAM Size          :  " << hex(header.ram_size) << "     (" << header.ram_size_description() << ")\n";
    std::cout << "CGB flag          :  " << hex(header.cgb_flag) << "     (" << header.cgb_flag_description() << ")\n";
    std::cout << "SGB flag          :  " << hex(header.sgb_flag) << "\n";
    std::cout << "Destination Code  :  " << hex(header.destination_code) << "\n";
    std::cout << "Rom Version Num.  :  " << hex(header.rom_version_number) << "\n";
    std::cout << "Header checksum   :  " << hex(header.header_checksum) << "\n";
}

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

template <uint32_t min_num = std::numeric_limits<double>::min(), uint32_t min_denum = 1,
          uint32_t max_num = std::numeric_limits<double>::max(), uint32_t max_denum = 1>
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

template <typename T, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()>
std::optional<T> parse_floating_point(const std::string& s) {
    const char* cstr = s.c_str();
    char* endptr {};
    errno = 0;

    auto val = std::strtod(cstr, &endptr);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

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

void ensure_file_exists(const std::string& path) {
    if (!file_exists(path)) {
        std::cerr << "ERROR: failed to load '" << path << "'" << std::endl;
        exit(1);
    }
}

std::string get_preferences_path() {
    const char* pref_path_cstr = SDL_GetPrefPath("", "DocBoy");
    std::string pref_path = (Path {pref_path_cstr} / "prefs.ini").string();
    return pref_path;
}

void read_preferences(const std::string& path, Preferences& p) {
    IniReader ini_reader;
    ini_reader.add_comment_prefix("#");
    ini_reader.add_property("dmg_palette", p.palette, parse_hex_array<4>);
    ini_reader.add_property("a", p.keys.a, parse_keycode);
    ini_reader.add_property("b", p.keys.b, parse_keycode);
    ini_reader.add_property("start", p.keys.start, parse_keycode);
    ini_reader.add_property("select", p.keys.select, parse_keycode);
    ini_reader.add_property("left", p.keys.left, parse_keycode);
    ini_reader.add_property("up", p.keys.up, parse_keycode);
    ini_reader.add_property("right", p.keys.right, parse_keycode);
    ini_reader.add_property("down", p.keys.down, parse_keycode);
    ini_reader.add_property("scaling", p.scaling, parse_integer<uint32_t>);
    ini_reader.add_property("x", p.x, parse_integer<int32_t>);
    ini_reader.add_property("y", p.y, parse_integer<int32_t>);

#ifdef ENABLE_AUDIO
    ini_reader.add_property("audio", p.audio, parse_bool);
    ini_reader.add_property("volume", p.volume, parse_integer<uint8_t, 0, 100>);
    ini_reader.add_property("dynamic_sample_rate_control", p.dynamic_sample_rate.enabled, parse_bool);
    ini_reader.add_property("dynamic_sample_rate_control_max_latency", p.dynamic_sample_rate.max_latency,
                            parse_integer<uint32_t>);
    ini_reader.add_property("dynamic_sample_rate_control_moving_average_factor",
                            p.dynamic_sample_rate.moving_average_factor, parse_double<0, 0, 1, 1>);
    ini_reader.add_property("dynamic_sample_rate_control_max_pitch_slack_factor",
                            p.dynamic_sample_rate.max_pitch_slack_factor, parse_double<0, 0, 1, 1>);
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

    properties.emplace("dmg_palette", join(p.palette, ",", [](uint16_t val) {
                           return hex(val);
                       }));
    properties.emplace("a", SDL_GetKeyName(p.keys.a));
    properties.emplace("b", SDL_GetKeyName(p.keys.b));
    properties.emplace("start", SDL_GetKeyName(p.keys.start));
    properties.emplace("select", SDL_GetKeyName(p.keys.select));
    properties.emplace("left", SDL_GetKeyName(p.keys.left));
    properties.emplace("up", SDL_GetKeyName(p.keys.up));
    properties.emplace("right", SDL_GetKeyName(p.keys.right));
    properties.emplace("down", SDL_GetKeyName(p.keys.down));
    properties.emplace("scaling", std::to_string(p.scaling));
    properties.emplace("x", std::to_string(p.x));
    properties.emplace("y", std::to_string(p.y));

#ifdef ENABLE_AUDIO
    properties.emplace("audio", std::to_string(p.audio));
    properties.emplace("volume", std::to_string(p.volume));
    properties.emplace("dynamic_sample_rate_control", std::to_string(p.dynamic_sample_rate.enabled));
    properties.emplace("dynamic_sample_rate_control_max_latency", std::to_string(p.dynamic_sample_rate.max_latency));
    properties.emplace("dynamic_sample_rate_control_moving_average_factor",
                       std::to_string(p.dynamic_sample_rate.moving_average_factor));
    properties.emplace("dynamic_sample_rate_control_max_pitch_slack_factor",
                       std::to_string(p.dynamic_sample_rate.max_pitch_slack_factor));
#endif

    IniWriter ini_writer;
    if (!ini_writer.write(properties, path)) {
        std::cerr << "WARN: failed to write '" << path << "'" << std::endl;
    }
}
} // namespace

int main(int argc, char* argv[]) {
    // Parse command line arguments
    struct {
        std::string rom;
#ifdef ENABLE_BOOTROM
        std::string boot_rom;
#endif
        std::string config {};
        bool serial {};
        uint32_t scaling {};
        bool dump_cartridge_info {};
#ifdef ENABLE_DEBUGGER
        bool attach_debugger {};
#endif
#ifdef ENABLE_AUDIO
        std::string audio_device_name {};
#endif
    } args;

    Args::Parser args_parser {};
#ifdef ENABLE_BOOTROM
    args_parser.add_argument(args.boot_rom, "boot-rom").help("Boot ROM");
#endif
    args_parser.add_argument(args.rom, "rom").required(false).help("ROM");
    args_parser.add_argument(args.config, "--config", "-c").help("Read configuration file");
#ifdef ENABLE_SERIAL
    args_parser.add_argument(args.serial, "--serial", "-s").help("Display serial console");
#endif
    args_parser.add_argument(args.scaling, "--scaling", "-z").help("Scaling factor");
    args_parser.add_argument(args.dump_cartridge_info, "--cartridge-info", "-i").help("Dump cartridge info and quit");
#ifdef ENABLE_DEBUGGER
    args_parser.add_argument(args.attach_debugger, "--debugger", "-d").help("Attach debugger");
#endif
#ifdef ENABLE_AUDIO
    // (mostly for development)
    args_parser.add_argument(args.audio_device_name, "--audio-device-name", "-a").help("Override output audio device");
#endif

    // Parse command line arguments
    if (!args_parser.parse(argc, argv, 1)) {
        return 1;
    }

    // Eventually just dump cartridge info and quit
    if (!args.rom.empty() && args.dump_cartridge_info) {
        dump_cartridge_info(*CartridgeFactory::create(args.rom));
        return 0;
    }

    // Create default preferences
    Preferences prefs {make_default_preferences()};

    // Eventually load preferences
    std::string pref_path {get_preferences_path()};
    if (file_exists(pref_path)) {
        read_preferences(pref_path, prefs);
    }

    // Eventually load configuration file (override preferences)
    if (!args.config.empty()) {
        ensure_file_exists(args.config);
        read_preferences(args.config, prefs);
    }

    // Args override both preferences and config
    if (args.scaling > 0) {
        prefs.scaling = args.scaling;
    }

    prefs.scaling = std::max(prefs.scaling, 1U);

#ifdef ENABLE_BOOTROM
    ensure_file_exists(args.boot_rom);
    if (file_size(args.boot_rom) != BootRom::Size) {
        std::cerr << "ERROR: invalid boot rom '" << args.boot_rom << "'" << std::endl;
        return 3;
    }
#endif

    // Initialize SDL
#ifdef ENABLE_AUDIO
    const uint32_t sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO;
#else
    const uint32_t sdl_init_flags = SDL_INIT_VIDEO;
#endif

    if (SDL_Init(sdl_init_flags) != 0) {
        std::cerr << "ERROR: SDL initialization failed '" << SDL_GetError() << "'" << std::endl;
        return 4;
    }

    //    audio_test();

#ifdef NFD
    // Initialize NFD
    if (NFD_Init() != NFD_OKAY) {
        std::cerr << "ERROR: NFD initialization failed '" << NFD_GetError() << "'" << std::endl;
        return 5;
    }
#endif

    // Create GameBoy and Core
#ifdef ENABLE_BOOTROM
    auto gb {std::make_unique<GameBoy>(BootRomFactory::create(args.boot_rom))};
#else
    auto gb {std::make_unique<GameBoy>()};
#endif

    Core core {*gb};

    // Create SDL window
    Window window {prefs.x, prefs.y, prefs.scaling};

    // Create screens' context
    CoreController core_controller {core};
    UiController ui_controller {window, core};
    NavController nav_controller {window.get_screen_stack()};
    MainController main_controller {};
#ifdef ENABLE_DEBUGGER
    DebuggerController debugger_controller {window, core, core_controller};
#endif

#ifdef ENABLE_DEBUGGER
    Screen::Controllers controllers {core_controller, ui_controller, nav_controller, main_controller,
                                     debugger_controller};
#else
    Screen::Controllers controllers {core_controller, ui_controller, nav_controller, main_controller};
#endif

    Screen::Context context {controllers, {0xFF}};

    // Keymap preferences
    core_controller.set_key_mapping(prefs.keys.a, Joypad::Key::A);
    core_controller.set_key_mapping(prefs.keys.b, Joypad::Key::B);
    core_controller.set_key_mapping(prefs.keys.start, Joypad::Key::Start);
    core_controller.set_key_mapping(prefs.keys.select, Joypad::Key::Select);
    core_controller.set_key_mapping(prefs.keys.left, Joypad::Key::Left);
    core_controller.set_key_mapping(prefs.keys.up, Joypad::Key::Up);
    core_controller.set_key_mapping(prefs.keys.right, Joypad::Key::Right);
    core_controller.set_key_mapping(prefs.keys.down, Joypad::Key::Down);

    // Scaling preferences
    ui_controller.set_scaling(prefs.scaling);

    // Palette preferences
    ui_controller.add_palette({0x84A0, 0x4B40, 0x2AA0, 0x1200}, "Green", 0xFFFF);
    ui_controller.add_palette({0xFFFF, 0xAD55, 0x52AA, 0x0000}, "Grey", 0xA901);
    const auto* palette = ui_controller.get_palette(prefs.palette);
    if (!palette) {
        // No know palette exists for this configuration: add a new one
        palette = &ui_controller.add_palette(prefs.palette, "User");
    }
    ui_controller.set_current_palette(palette->index);

#ifdef ENABLE_SERIAL
    // Eventually attach serial
    std::unique_ptr<SerialConsole> serial_console;
    std::unique_ptr<SerialLink> serial_link;
    if (args.serial) {
        serial_console = std::make_unique<SerialConsole>(std::cerr, 16);
        serial_link = std::make_unique<SerialLink>();
        serial_link->plug1.attach(*serial_console);
        core.attach_serial_link(serial_link->plug2);
    }
#endif

#ifdef ENABLE_DEBUGGER
    // Eventually attach debugger
    if (args.attach_debugger) {
        debugger_controller.attach_debugger();
    }
#endif

#ifdef ENABLE_AUDIO
    const SDL_AudioSpec audio_device_spec_hint = {SDL_AUDIO_S16, Apu::NUM_CHANNELS, 32768};

    // Eventually use user-specified audio device
    SDL_AudioDeviceID audio_output_device_id {SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK};

    if (!args.audio_device_name.empty()) {
        int count {};
        SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);

        int i;
        for (i = 0; i < count; i++) {
            const auto device_id = devices[i];
            if (SDL_GetAudioDeviceName(device_id) == args.audio_device_name) {
                audio_output_device_id = device_id;
                break;
            }
        }

        if (i == count) {
            std::cerr << "WARN: failed to find audio device '" << args.audio_device_name << "'; using default one"
                      << std::endl;
        }
    }

    // Open audio device
    const SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(audio_output_device_id, &audio_device_spec_hint);
    if (!audio_device) {
        std::cerr << "ERROR: SDL audio device initialization failed '" << SDL_GetError() << "'" << std::endl;
        return 6;
    }

    SDL_AudioSpec audio_device_spec;
    if (SDL_GetAudioDeviceFormat(audio_device, &audio_device_spec, nullptr) != 0) {
        std::cerr << "ERROR: failed to retrieve format of SDL audio device '" << SDL_GetError() << "'" << std::endl;
        return 7;
    }

    const uint32_t audio_device_sample_rate = audio_device_spec.freq;

    const SDL_AudioSpec audio_stream_spec {SDL_AUDIO_S16, Apu::NUM_CHANNELS,
                                           static_cast<int>(audio_device_sample_rate)};

    // Create audio stream
    SDL_AudioStream* audio_stream = SDL_CreateAudioStream(&audio_stream_spec, &audio_device_spec);
    if (!audio_stream) {
        std::cerr << "ERROR: SDL audio stream initialization failed '" << SDL_GetError() << "'" << std::endl;
        return 8;
    }

    // Set audio sample rate accordingly to opened audio device
    core.set_audio_sample_rate(audio_device_sample_rate);

    bool audio_stream_bound {};

    const auto unbind_audio_stream = [&audio_stream_bound, &audio_stream]() {
        SDL_UnbindAudioStream(audio_stream);
        audio_stream_bound = false;
    };

    const auto bind_audio_stream = [&audio_stream_bound, &audio_device, &audio_stream]() {
        SDL_BindAudioStream(audio_device, audio_stream);
        audio_stream_bound = true;
    };

    // Audio preferences
    main_controller.set_audio_enabled(prefs.audio);
    main_controller.set_audio_enabled_changed_callback([&unbind_audio_stream](bool enabled) {
        // Unbind audio stream when audio is disabled.
        if (!enabled) {
            unbind_audio_stream();
        }
    });

    main_controller.set_volume_changed_callback([&core](uint8_t volume /* [0, 100] */) {
        core.set_audio_volume(static_cast<float>(volume) / 100);
    });
    main_controller.set_volume(prefs.volume);

    struct {
        bool enabled {};
        struct {
            double alpha {};
            double beta {};
        } average_factors;
        struct {
            double lower {};
            double upper {};
        } pitch_slack_bounds;
        double target_sample_queue_size {};
    } dynamic_sample_rate_control;

    const auto compute_dynamic_sample_rate_control_settings = [&main_controller, &dynamic_sample_rate_control,
                                                               audio_device_sample_rate]() {
        const double avg_factor = main_controller.get_dynamic_sample_rate_control_moving_average_factor();
        const double max_pitch_slack_factor = main_controller.get_dynamic_sample_rate_control_max_pitch_slack_factor();

        dynamic_sample_rate_control.enabled = main_controller.is_dynamic_sample_rate_control_enabled();

        dynamic_sample_rate_control.average_factors.alpha = 1.0 - avg_factor;
        dynamic_sample_rate_control.average_factors.beta = avg_factor;

        dynamic_sample_rate_control.pitch_slack_bounds.lower = 1.0 - max_pitch_slack_factor;
        dynamic_sample_rate_control.pitch_slack_bounds.upper = 1.0 + max_pitch_slack_factor;

        dynamic_sample_rate_control.target_sample_queue_size =
            (double)audio_device_sample_rate * Apu::NUM_CHANNELS * sizeof(int16_t) *
            (main_controller.get_dynamic_sample_rate_control_max_latency() / 1000);
    };

    main_controller.set_dynamic_sample_rate_control_enabled(prefs.dynamic_sample_rate.enabled);
    main_controller.set_dynamic_sample_rate_control_max_latency(prefs.dynamic_sample_rate.max_latency);
    main_controller.set_dynamic_sample_rate_control_moving_average_factor(
        prefs.dynamic_sample_rate.moving_average_factor);
    main_controller.set_dynamic_sample_rate_control_max_pitch_slack_factor(
        prefs.dynamic_sample_rate.max_pitch_slack_factor);

    main_controller.set_dynamic_sample_rate_control_settings_changed(compute_dynamic_sample_rate_control_settings);

    // Compute current dynamic sample rate settings
    compute_dynamic_sample_rate_control_settings();

    struct {
        int16_t data[65536] {};
        uint16_t index {};
    } audiobuffer;

    double sample_rate_control = 1.0;

    core.set_audio_sample_callback([&audiobuffer](const Apu::AudioSample sample) {
        if (audiobuffer.index < array_size(audiobuffer.data) - 1) {
            audiobuffer.data[audiobuffer.index++] = sample.left;
            audiobuffer.data[audiobuffer.index++] = sample.right;
        }
    });
#endif

    core_controller.set_paused_changed_callback([&unbind_audio_stream](bool paused) {
#ifdef ENABLE_AUDIO
        // Unbind audio stream when game is paused.
        if (paused) {
            unbind_audio_stream();
        }
#endif
    });

    if (!args.rom.empty()) {
        // Start with loaded game
        core_controller.load_rom(args.rom);
        nav_controller.push(std::make_unique<GameScreen>(context));
    } else {
        // Start with launcher screen instead
        nav_controller.push(std::make_unique<LauncherScreen>(context));
    }

    // Main loop
    SDL_Event e;
    std::chrono::high_resolution_clock::time_point next_frame_time = std::chrono::high_resolution_clock::now();

#ifdef ENABLE_DEBUGGER
    while (!main_controller.should_quit() && !core.is_asking_to_quit()) {
#else
    while (!main_controller.should_quit()) {
#endif
        // Wait until next frame
        while (std::chrono::high_resolution_clock::now() < next_frame_time) {
        }
        next_frame_time += main_controller.get_frame_time();

        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                main_controller.quit();
                break;
            default:
                window.handle_event(e);
            }
        }

        // Eventually advance emulation by one frame
        if (!core_controller.is_paused()) {
            core_controller.frame();

#ifdef ENABLE_AUDIO
            if (audio_stream_bound) {
                if (dynamic_sample_rate_control.enabled) {
                    // Adjust the APU sample rate based on the real queued sampled and the desired queued sampled.
                    // We want the queue to have enough (to avoid underrun) but not
                    // too many samples (avoid latency between video and audio).
                    const uint32_t sample_queue_size = SDL_GetAudioStreamQueued(audio_stream);

                    const double immediate_sample_rate_control = dynamic_sample_rate_control.target_sample_queue_size /
                                                                 (sample_queue_size > 0 ? sample_queue_size : 1.0);

                    const double target_sample_rate_control =
                        dynamic_sample_rate_control.average_factors.alpha * sample_rate_control +
                        dynamic_sample_rate_control.average_factors.beta * immediate_sample_rate_control;

                    sample_rate_control =
                        std::clamp(target_sample_rate_control, dynamic_sample_rate_control.pitch_slack_bounds.lower,
                                   dynamic_sample_rate_control.pitch_slack_bounds.upper);

                    const double sample_rate = audio_device_sample_rate * sample_rate_control;
                    core.set_audio_sample_rate(sample_rate);
                }
            } else if (main_controller.is_audio_enabled()) {
                // Bind the audio stream if we have enough samples to start and audio is enabled.
                if (!dynamic_sample_rate_control.enabled ||
                    audiobuffer.index * sizeof(int16_t) >
                        static_cast<uint32_t>(dynamic_sample_rate_control.target_sample_queue_size)) {
                    bind_audio_stream();
                }
            }

            if (audio_stream_bound) {
                // Send the audio samples we've collected.
                SDL_PutAudioStreamData(audio_stream, audiobuffer.data,
                                       static_cast<int>(audiobuffer.index * sizeof(int16_t)));
                audiobuffer.index = 0;
            }
#endif
        }

        // Render current frame
        window.render();
    }

    // Write RAM state
    if (core_controller.is_rom_loaded()) {
        core_controller.write_save();
    }

    // Write current preferences
    prefs.palette = ui_controller.get_current_palette().rgb565.palette;
    prefs.scaling = window.get_scaling();

    const auto& keycodes {core_controller.get_joypad_map()};
    prefs.keys.a = keycodes.at(Joypad::Key::A);
    prefs.keys.b = keycodes.at(Joypad::Key::B);
    prefs.keys.start = keycodes.at(Joypad::Key::Start);
    prefs.keys.select = keycodes.at(Joypad::Key::Select);
    prefs.keys.left = keycodes.at(Joypad::Key::Left);
    prefs.keys.up = keycodes.at(Joypad::Key::Up);
    prefs.keys.right = keycodes.at(Joypad::Key::Right);
    prefs.keys.down = keycodes.at(Joypad::Key::Down);

    Window::Position pos = window.get_position();
    prefs.x = pos.x;
    prefs.y = pos.y;

#ifdef ENABLE_AUDIO
    prefs.audio = main_controller.is_audio_enabled();
    prefs.volume = main_controller.get_volume();

    prefs.dynamic_sample_rate.enabled = main_controller.is_dynamic_sample_rate_control_enabled();
    prefs.dynamic_sample_rate.max_latency =
        static_cast<uint32_t>(main_controller.get_dynamic_sample_rate_control_max_latency());
    prefs.dynamic_sample_rate.moving_average_factor =
        main_controller.get_dynamic_sample_rate_control_moving_average_factor();
    prefs.dynamic_sample_rate.max_pitch_slack_factor =
        main_controller.get_dynamic_sample_rate_control_max_pitch_slack_factor();
#endif

    write_preferences(pref_path, prefs);

#ifdef ENABLE_SERIAL
    if (serial_console) {
        serial_console->flush();
    }
#endif

#ifdef ENABLE_AUDIO
    // Release audio resources
    SDL_DestroyAudioStream(audio_stream);
    SDL_CloseAudioDevice(audio_device);
#endif

    return 0;
}