#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>

#include "SDL3/SDL.h"

#include "docboy/cartridge/factory.h"

#include "controllers/corecontroller.h"
#include "controllers/maincontroller.h"
#include "controllers/runcontroller.h"
#include "controllers/uicontroller.h"
#include "preferences/preferences.h"
#include "screens/gamescreen.h"
#include "screens/launcherscreen.h"
#include "windows/window.h"

#include "extra/cartridge/header.h"
#include "extra/serial/endpoints/console.h"

#include "args/args.h"

#include "utils/formatters.h"
#include "utils/os.h"

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
constexpr UiController::LcdAppearance DMG_PALETTE {0xAD00, {0x84A0, 0x4B40, 0x2AA0, 0x1200}};
constexpr UiController::LcdAppearance BRIGHT_GREEN_PALETTE {0x84A0, {0x84A0, 0x4B40, 0x2AA0, 0x1200}};
constexpr UiController::LcdAppearance DULL_GREEN_PALETTE {0x9CE7, {0x9CE7, 0x4B44, 0x0A21, 0x1941}};
constexpr UiController::LcdAppearance GREY_PALETTE {0xFFFF, {0xFFFF, 0xAD55, 0x52AA, 0x0000}};

Preferences make_default_preferences() {
    Preferences prefs {};
#ifndef ENABLE_CGB
    prefs.dmg_palette = DMG_PALETTE;
#endif
    prefs.keys.player1.a = SDLK_Z;
    prefs.keys.player1.b = SDLK_X;
    prefs.keys.player1.start = SDLK_RETURN;
    prefs.keys.player1.select = SDLK_TAB;
    prefs.keys.player1.left = SDLK_LEFT;
    prefs.keys.player1.up = SDLK_UP;
    prefs.keys.player1.right = SDLK_RIGHT;
    prefs.keys.player1.down = SDLK_DOWN;
#ifdef ENABLE_TWO_PLAYERS_MODE
    prefs.keys.player2.a = SDLK_A;
    prefs.keys.player2.b = SDLK_S;
    prefs.keys.player2.start = SDLK_RSHIFT;
    prefs.keys.player2.select = SDLK_LSHIFT;
    prefs.keys.player2.left = SDLK_J;
    prefs.keys.player2.up = SDLK_I;
    prefs.keys.player2.right = SDLK_L;
    prefs.keys.player2.down = SDLK_K;
#endif
    prefs.scaling = 2;
    prefs.scaling_filter = UiController::ScalingFilter::NearestNeighbor;
    prefs.x = 200;
    prefs.y = 200;
#ifdef ENABLE_AUDIO
    prefs.audio = true;
#ifdef ENABLE_TWO_PLAYERS_MODE
    prefs.audio_player_source = 1;
#endif
    prefs.volume = 100;
    prefs.high_pass_filter = true;
    prefs.dynamic_sample_rate.enabled = true;
    prefs.dynamic_sample_rate.max_latency = 50;
    prefs.dynamic_sample_rate.moving_average_factor = 0.005;
    prefs.dynamic_sample_rate.max_pitch_slack_factor = 0.005;
#endif
#ifdef ENABLE_TWO_PLAYERS_MODE
    prefs.serial_link = true;
#endif
    return prefs;
}

void dump_cartridge_info(const CartridgeHeader& header) {
    using namespace CartridgeHeaderHelpers;

    std::cout << "Title             :  " << title_as_string(header) << "\n";
    std::cout << "Cartridge Type    :  " << hex(header.cartridge_type) << "     (" << cartridge_type_description(header)
              << ")\n";
    std::cout << "Licensee (old)    :  " << hex(header.old_licensee_code) << "     ("
              << old_licensee_code_description(header) << ")\n";
    std::cout << "Licensee (new)    :  " << new_licensee_code_as_string(header) << "     ("
              << new_licensee_code_description(header) << ")\n";
    std::cout << "ROM Size          :  " << hex(header.rom_size) << "     (" << rom_size_description(header) << ")\n";
    std::cout << "RAM Size          :  " << hex(header.ram_size) << "     (" << ram_size_description(header) << ")\n";
    std::cout << "CGB Flag          :  " << hex(cgb_flag(header)) << "     (" << cgb_flag_description(header) << ")\n";
    std::cout << "SGB Flag          :  " << hex(header.sgb_flag) << "\n";
    std::cout << "Destination Code  :  " << hex(header.destination_code) << "\n";
    std::cout << "Rom Version Num.  :  " << hex(header.rom_version_number) << "\n";
    std::cout << "Header Checksum   :  " << hex(header.header_checksum) << "\n";
    std::cout << "Title Checksum    :  " << hex(title_checksum(header)) << "\n";
}

void ensure_file_exists(const std::string& path) {
    if (!file_exists(path)) {
        std::cerr << "ERROR: failed to load '" << path << "'" << std::endl;
        exit(1);
    }
}

void list_audio_devices() {
    int count {};

    SDL_AudioDeviceID* devices_ids = SDL_GetAudioPlaybackDevices(&count);
    if (!devices_ids) {
        std::cerr << "ERROR: SDL failed to retrieve audio devices: '" << SDL_GetError() << "'" << std::endl;
        exit(1);
    }

    std::sort(devices_ids, devices_ids + count);

    for (int i = 0; i < count; i++) {
        SDL_AudioDeviceID device_id = devices_ids[i];
        const char* device_name = SDL_GetAudioDeviceName(device_id);
        std::cout << device_id << ": " << (device_name ? device_name : "Unknown") << std::endl;
    }

    SDL_free(devices_ids);
}

// GameBoy and Core
GameBoy gb {};
Core core {gb};

#ifdef ENABLE_TWO_PLAYERS_MODE
// Second GameBoy and Core
std::unique_ptr<GameBoy> gb2;
std::unique_ptr<Core> core2;
#endif
} // namespace

int main(int argc, char* argv[]) {
    // Parse command line arguments
    struct {
        std::string rom {};
#ifdef ENABLE_TWO_PLAYERS_MODE
        std::optional<std::string> second_rom;
#endif
#ifdef ENABLE_BOOTROM
        std::string boot_rom;
#endif
        std::string config {};
        bool serial_console {};
        uint32_t scaling {};
        bool dump_cartridge_info {};
#ifdef ENABLE_DEBUGGER
        bool attach_debugger {};
#endif
#ifdef ENABLE_AUDIO
        uint32_t audio_device_id {SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK};
        bool list_audio_devices {};
#endif
    } args;

    Args::Parser args_parser {};
#ifdef ENABLE_BOOTROM
    args_parser.add_argument(args.boot_rom, "boot-rom").help("Boot ROM");
#endif
    args_parser.add_argument(args.rom, "rom").required(false).help("ROM");
#ifdef ENABLE_TWO_PLAYERS_MODE
    args_parser.add_argument(args.second_rom, "--second-rom", "-2").help("Second player rom");
#endif
    args_parser.add_argument(args.config, "--config", "-c").help("Read configuration file");
    args_parser.add_argument(args.serial_console, "--serial-console", "-s").help("Display serial console");
    args_parser.add_argument(args.scaling, "--scaling", "-z").help("Scaling factor");
    args_parser.add_argument(args.dump_cartridge_info, "--cartridge-info", "-i").help("Dump cartridge info and quit");
#ifdef ENABLE_DEBUGGER
    args_parser.add_argument(args.attach_debugger, "--debugger", "-d").help("Attach debugger");
#endif
#ifdef ENABLE_AUDIO
    // (mostly for development)
    args_parser.add_argument(args.audio_device_id, "--audio-device", "-a").help("Set preferred audio device");
    args_parser.add_argument(args.list_audio_devices, "--list-audio-devices", "-l").help("List audio devices");
#endif

    // Parse command line arguments
    if (!args_parser.parse(argc, argv, 1)) {
        return 1;
    }

    // Eventually just dump cartridge info and quit
    if (!args.rom.empty() && args.dump_cartridge_info) {
        dump_cartridge_info(CartridgeFactory::create_header(args.rom));
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
        return 1;
    }
#endif

    // Initialize SDL
#ifdef ENABLE_AUDIO
    constexpr uint32_t sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO;
#else
    const uint32_t sdl_init_flags = SDL_INIT_VIDEO;
#endif

    if (!SDL_Init(sdl_init_flags)) {
        std::cerr << "ERROR: SDL initialization failed: '" << SDL_GetError() << "'" << std::endl;
        return 1;
    }

    // Eventually just list audio devices and quit
    if (args.list_audio_devices) {
        list_audio_devices();
        return 0;
    }

#ifdef NFD
    // Initialize NFD
    if (NFD_Init() != NFD_OKAY) {
        std::cerr << "ERROR: NFD initialization failed: '" << NFD_GetError() << "'" << std::endl;
        return 1;
    }
#endif

    bool two_players_mode = false;

#ifdef ENABLE_TWO_PLAYERS_MODE
    // Eventually create second GameBoy and Core
    two_players_mode = args.second_rom.has_value();

    if (two_players_mode) {
        gb2 = std::make_unique<GameBoy>();
        core2 = std::make_unique<Core>(*gb2);
    }
#endif

    // Create SDL window
    int width = Specs::Display::WIDTH;
    int height = Specs::Display::HEIGHT;

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (two_players_mode) {
        width *= 2;
    }
#endif

    Window window {width, height, prefs.x, prefs.y, prefs.scaling};

    // Create screens' context

#ifdef ENABLE_TWO_PLAYERS_MODE
    RunController run_controller {core, core2 ? &*core2 : nullptr};
#else
    RunController run_controller {core};
#endif
    UiController ui_controller {window, run_controller};
    NavController nav_controller {window.get_screen_stack()};
    MainController main_controller {};

    CoreController& core_controller1 {run_controller.get_core1()};
#ifdef ENABLE_TWO_PLAYERS_MODE
    CoreController* core_controller2 {two_players_mode ? &run_controller.get_core2() : nullptr};
#endif
#ifdef ENABLE_DEBUGGER
    DebuggerController debugger_controller {window, core, core_controller1};
#endif

#ifdef ENABLE_DEBUGGER
    Screen::Controllers controllers {run_controller, ui_controller, nav_controller, main_controller,
                                     debugger_controller};
#else
    Screen::Controllers controllers {run_controller, ui_controller, nav_controller, main_controller};
#endif

    Screen::Context context {controllers, {0xFF}};

    // Keymap preferences
    core_controller1.set_key_mapping(prefs.keys.player1.a, Joypad::Key::A);
    core_controller1.set_key_mapping(prefs.keys.player1.b, Joypad::Key::B);
    core_controller1.set_key_mapping(prefs.keys.player1.start, Joypad::Key::Start);
    core_controller1.set_key_mapping(prefs.keys.player1.select, Joypad::Key::Select);
    core_controller1.set_key_mapping(prefs.keys.player1.left, Joypad::Key::Left);
    core_controller1.set_key_mapping(prefs.keys.player1.up, Joypad::Key::Up);
    core_controller1.set_key_mapping(prefs.keys.player1.right, Joypad::Key::Right);
    core_controller1.set_key_mapping(prefs.keys.player1.down, Joypad::Key::Down);

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (two_players_mode) {
        core_controller2->set_key_mapping(prefs.keys.player2.a, Joypad::Key::A);
        core_controller2->set_key_mapping(prefs.keys.player2.b, Joypad::Key::B);
        core_controller2->set_key_mapping(prefs.keys.player2.start, Joypad::Key::Start);
        core_controller2->set_key_mapping(prefs.keys.player2.select, Joypad::Key::Select);
        core_controller2->set_key_mapping(prefs.keys.player2.left, Joypad::Key::Left);
        core_controller2->set_key_mapping(prefs.keys.player2.up, Joypad::Key::Up);
        core_controller2->set_key_mapping(prefs.keys.player2.right, Joypad::Key::Right);
        core_controller2->set_key_mapping(prefs.keys.player2.down, Joypad::Key::Down);
    }
#endif

    // Scaling preferences
    ui_controller.set_scaling(prefs.scaling);
    ui_controller.set_scaling_filter(prefs.scaling_filter);

    // Palette preferences
#ifdef ENABLE_CGB
    // TODO: CGB palette?
    const auto& appearance = ui_controller.add_appearance(DMG_PALETTE, "DMG", 0xFFFF);
    ui_controller.set_current_appearance(appearance.index);
#else
    ui_controller.add_appearance(DMG_PALETTE, "DMG", 0xFFFF);
    ui_controller.add_appearance(BRIGHT_GREEN_PALETTE, "Bright", 0xFFFF);
    ui_controller.add_appearance(DULL_GREEN_PALETTE, "Dull", 0xFFFF);
    ui_controller.add_appearance(GREY_PALETTE, "Grey", 0xA901);
    const auto* appearance = ui_controller.get_appearance(prefs.dmg_palette);
    if (!appearance) {
        // No know appearance exists for this configuration: add a new one
        appearance = &ui_controller.add_appearance(prefs.dmg_palette, "User");
    }
    ui_controller.set_current_appearance(appearance->index);
#endif

    // Eventually attach serial console
    std::unique_ptr<SerialConsole> serial_console;
    if (!two_players_mode && args.serial_console) {
        serial_console = std::make_unique<SerialConsole>(std::cerr, 16);
        core.attach_serial_link(*serial_console);
    }

    // Eventually attach serial link in two players mode
#ifdef ENABLE_TWO_PLAYERS_MODE
    if (two_players_mode && prefs.serial_link) {
        run_controller.attach_serial_link();
    }
#endif

#ifdef ENABLE_DEBUGGER
    // Eventually attach debugger
    if (args.attach_debugger) {
        debugger_controller.attach_debugger();
    }
#endif

#ifdef ENABLE_AUDIO
    static constexpr SDL_AudioSpec AUDIO_DEVICE_SPEC_HINT {SDL_AUDIO_S16, Apu::NUM_CHANNELS, 32768};

    // Eventually use user-specified audio device
    SDL_AudioDeviceID audio_output_device_id = args.audio_device_id;

    // Open audio device
    const SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(audio_output_device_id, &AUDIO_DEVICE_SPEC_HINT);
    if (!audio_device) {
        std::cerr << "ERROR: SDL audio device initialization failed: '" << SDL_GetError() << "'" << std::endl;
        return 1;
    }

    SDL_AudioSpec audio_device_spec;
    if (!SDL_GetAudioDeviceFormat(audio_device, &audio_device_spec, nullptr)) {
        std::cerr << "ERROR: failed to retrieve format of SDL audio device: '" << SDL_GetError() << "'" << std::endl;
        return 1;
    }

    const uint32_t audio_device_sample_rate = audio_device_spec.freq;

    const SDL_AudioSpec audio_stream_spec {SDL_AUDIO_S16, Apu::NUM_CHANNELS,
                                           static_cast<int>(audio_device_sample_rate)};

    // Create audio stream
    SDL_AudioStream* audio_stream = SDL_CreateAudioStream(&audio_stream_spec, &audio_device_spec);
    if (!audio_stream) {
        std::cerr << "ERROR: SDL audio stream initialization failed: '" << SDL_GetError() << "'" << std::endl;
        return 1;
    }

    // Set audio sample rate accordingly to opened audio device
    core.set_audio_sample_rate(audio_device_sample_rate);

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (two_players_mode) {
        core2->set_audio_sample_rate(audio_device_sample_rate);
    }
#endif

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

#ifdef ENABLE_TWO_PLAYERS_MODE
    main_controller.set_audio_player_source(prefs.audio_player_source);
#endif

    main_controller.set_volume_changed_callback([two_players_mode](uint8_t volume /* [0, 100] */) {
        core.set_audio_volume(static_cast<double>(volume) / 100);

#ifdef ENABLE_TWO_PLAYERS_MODE
        if (two_players_mode) {
            core2->set_audio_volume(static_cast<double>(volume) / 100);
        }
#endif
    });
    main_controller.set_volume(prefs.volume);

    main_controller.set_high_pass_filter_enabled_changed_callback([two_players_mode](bool enabled) {
        core.set_audio_high_pass_filter_enabled(enabled);

#ifdef ENABLE_TWO_PLAYERS_MODE
        if (two_players_mode) {
            core2->set_audio_high_pass_filter_enabled(enabled);
        }
#endif
    });
    main_controller.set_high_pass_filter_enabled(prefs.high_pass_filter);

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

    // Audio sample callback
    const auto set_audio_sample_callback = [two_players_mode, &audiobuffer, &main_controller]() {
        auto audio_sample_callback = [&audiobuffer](const Apu::AudioSample sample) {
            if (audiobuffer.index < array_size(audiobuffer.data) - 1) {
                audiobuffer.data[audiobuffer.index++] = sample.left;
                audiobuffer.data[audiobuffer.index++] = sample.right;
            }
        };

        if (two_players_mode) {
            if (main_controller.get_audio_player_source() == MainController::AUDIO_PLAYER_SOURCE_1) {
                core.set_audio_sample_callback(std::move(audio_sample_callback));
                core2->set_audio_sample_callback(nullptr);
            } else {
                core.set_audio_sample_callback(nullptr);
                core2->set_audio_sample_callback(std::move(audio_sample_callback));
            }
        } else {
            core.set_audio_sample_callback(std::move(audio_sample_callback));
        }
    };

    set_audio_sample_callback();

#ifdef ENABLE_TWO_PLAYERS_MODE
    main_controller.set_audio_player_source_changed_callback([&set_audio_sample_callback](uint8_t player) {
        set_audio_sample_callback();
    });
#endif

#endif

#ifdef ENABLE_AUDIO
    run_controller.set_paused_changed_callback([&unbind_audio_stream](bool paused) {
        // Unbind audio stream when game is paused.
        if (paused) {
            unbind_audio_stream();
        }
    });
#endif

    // Load Boot ROM(s)
#ifdef ENABLE_BOOTROM
    core_controller1.load_boot_rom(args.boot_rom);

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (two_players_mode) {
        core_controller2->load_boot_rom(args.boot_rom);
    }
#endif
#endif

    // Load ROM(s)
    bool all_roms_loaded = false;

#ifdef ENABLE_TWO_PLAYERS_MODE
    all_roms_loaded = !args.rom.empty() && (!args.second_rom || !args.second_rom->empty());
#else
    all_roms_loaded = !args.rom.empty();
#endif

    if (!args.rom.empty()) {
        core_controller1.load_rom(args.rom);
    }

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (args.second_rom && !args.second_rom->empty()) {
        core_controller2->load_rom(*args.second_rom);
    }
#endif

    if (all_roms_loaded) {
        // Start with loaded game
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
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                main_controller.quit();
                break;
            default:
                window.handle_event(e);
            }
        }

        // Eventually advance emulation by one frame
        if (!run_controller.is_paused()) {
            run_controller.frame();

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
    if (core_controller1.is_rom_loaded()) {
        core_controller1.write_save();
    }

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (two_players_mode) {
        if (core_controller2->is_rom_loaded()) {
            core_controller2->write_save();
        }
    }
#endif

    // Write current preferences
#ifndef ENABLE_CGB
    prefs.dmg_palette = ui_controller.get_current_appearance().lcd;
#endif
    prefs.scaling = ui_controller.get_scaling();
    prefs.scaling_filter = ui_controller.get_scaling_filter();

    const auto update_preferences_keys_from_joypad_mapping =
        [](Preferences::Keys& keys, const std::map<Joypad::Key, SDL_Keycode>& joypad_mapping) {
            keys.a = joypad_mapping.at(Joypad::Key::A);
            keys.b = joypad_mapping.at(Joypad::Key::B);
            keys.start = joypad_mapping.at(Joypad::Key::Start);
            keys.select = joypad_mapping.at(Joypad::Key::Select);
            keys.left = joypad_mapping.at(Joypad::Key::Left);
            keys.up = joypad_mapping.at(Joypad::Key::Up);
            keys.right = joypad_mapping.at(Joypad::Key::Right);
            keys.down = joypad_mapping.at(Joypad::Key::Down);
        };

    update_preferences_keys_from_joypad_mapping(prefs.keys.player1, core_controller1.get_joypad_map());

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (two_players_mode) {
        update_preferences_keys_from_joypad_mapping(prefs.keys.player2, core_controller2->get_joypad_map());
    }
#endif

    Window::Position pos = window.get_position();
    prefs.x = pos.x;
    prefs.y = pos.y;

#ifdef ENABLE_AUDIO
    prefs.audio = main_controller.is_audio_enabled();
#ifdef ENABLE_TWO_PLAYERS_MODE
    prefs.audio_player_source = main_controller.get_audio_player_source();
#endif
    prefs.volume = main_controller.get_volume();
    prefs.high_pass_filter = main_controller.is_high_pass_filter_enabled();

    prefs.dynamic_sample_rate.enabled = main_controller.is_dynamic_sample_rate_control_enabled();
    prefs.dynamic_sample_rate.max_latency =
        static_cast<uint32_t>(main_controller.get_dynamic_sample_rate_control_max_latency());
    prefs.dynamic_sample_rate.moving_average_factor =
        main_controller.get_dynamic_sample_rate_control_moving_average_factor();
    prefs.dynamic_sample_rate.max_pitch_slack_factor =
        main_controller.get_dynamic_sample_rate_control_max_pitch_slack_factor();
#endif

#ifdef ENABLE_TWO_PLAYERS_MODE
    if (two_players_mode) {
        prefs.serial_link = run_controller.is_serial_link_attached();
    } // else: leave it as it was
#endif

    write_preferences(pref_path, prefs);

    // Eventually flush serial
    if (serial_console) {
        serial_console->flush();
    }

#ifdef ENABLE_AUDIO
    // Release audio resources
    SDL_DestroyAudioStream(audio_stream);
    SDL_CloseAudioDevice(audio_device);
#endif

    return 0;
}