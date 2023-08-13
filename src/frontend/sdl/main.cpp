#include "argparser.h"
#include "core/boot/bootromfactory.h"
#include "core/cartridge/cartridgefactory.h"
#include "core/config/config.h"
#include "core/config/parser.h"
#include "core/core.h"
#include "core/gameboy.h"
#include "core/serial/endpoints/console.h"
#include "core/state/state.h"
#include "helpers.h"
#include "utils/fileutils.h"
#include "utils/iniutils.h"
#include "window.h"
#include <SDL.h>
#include <filesystem>
#include <iostream>

#ifdef ENABLE_DEBUGGER
#include "core/debugger/backend.h"
#include "core/debugger/core/core.h"
#include "core/debugger/frontendcli.h"
#include "core/debugger/gameboy.h"
#endif

#ifdef ENABLE_PROFILER
#include "core/profiler/profiler.h"
#endif

static bool screenshot_bmp(uint32_t* framebuffer, const std::filesystem::path& path) {
    if (screenshot(framebuffer, Specs::Display::WIDTH, Specs::Display::HEIGHT, SDL_PIXELFORMAT_RGBA8888, path)) {
        std::cout << "Screenshot saved to: " << path << std::endl;
        return true;
    }
    return false;
}

static bool screenshot_dat(uint32_t* framebuffer, const std::filesystem::path& path) {
    bool ok;
    write_file(path, framebuffer, sizeof(uint32_t) * Specs::Display::WIDTH * Specs::Display::HEIGHT, &ok);
    if (ok)
        std::cout << "Screenshot saved to: " << path << std::endl;
    return ok;
}

static bool save_state(const IStataData& state, const std::filesystem::path& path) {
    const std::vector<uint8_t>& stateDate = state.getData();
    bool ok;
    write_file(path, (void*)stateDate.data(), stateDate.size(), &ok);
    if (ok)
        std::cout << "State saved to: " << path << std::endl;
    return ok;
}

static std::optional<State> load_state(const std::filesystem::path& path) {
    bool ok;
    const std::vector<uint8_t> data = read_file(path, &ok);
    if (!ok)
        return std::nullopt;
    std::cout << "State loaded from: " << path << std::endl;
    return State(data);
}

static void dump_cartridge_info(const ICartridge& cartridge) {
    const auto header = cartridge.header();
    std::cout << "Title             :  " << header.titleAsString() << "\n";
    std::cout << "Cartridge type    :  " << hex(header.cartridge_type) << "     (" << header.cartridgeTypeDescription()
              << ")\n";
    std::cout << "Licensee (new)    :  " << hex(header.new_licensee_code) << "  ("
              << header.newLicenseeCodeDescription() << ")\n";
    std::cout << "Licensee (old)    :  " << hex(header.old_licensee_code) << "     ("
              << header.oldLicenseeCodeDescription() << ")\n";
    std::cout << "ROM Size          :  " << hex(header.rom_size) << "     (" << header.romSizeDescription() << ")\n";
    std::cout << "RAM Size          :  " << hex(header.ram_size) << "     (" << header.ramSizeDescription() << ")\n";
    std::cout << "CGB flag          :  " << hex(header.cgb_flag) << "     (" << header.cgbFlagDescription() << ")\n";
    std::cout << "SGB flag          :  " << hex(header.sgb_flag) << "\n";
    std::cout << "Destination Code  :  " << hex(header.destination_code) << "\n";
    std::cout << "Rom Version Num.  :  " << hex(header.rom_version_number) << "\n";
    std::cout << "Header checksum   :  " << hex(header.header_checksum) << "\n";
}

static std::map<Config::Input::KeyboardKey, SDL_Keycode> KEYBOARD_KEYS_TO_SDL_KEYS = {
    {Config::Input::KeyboardKey::Return, SDLK_RETURN},
    {Config::Input::KeyboardKey::Escape, SDLK_ESCAPE},
    {Config::Input::KeyboardKey::Backspace, SDLK_BACKSPACE},
    {Config::Input::KeyboardKey::Tab, SDLK_TAB},
    {Config::Input::KeyboardKey::Space, SDLK_SPACE},
    {Config::Input::KeyboardKey::Exclaim, SDLK_EXCLAIM},
    {Config::Input::KeyboardKey::DoubleQuote, SDLK_QUOTEDBL},
    {Config::Input::KeyboardKey::Hash, SDLK_HASH},
    {Config::Input::KeyboardKey::Percent, SDLK_PERCENT},
    {Config::Input::KeyboardKey::Dollar, SDLK_DOLLAR},
    {Config::Input::KeyboardKey::Ampersand, SDLK_AMPERSAND},
    {Config::Input::KeyboardKey::Quote, SDLK_QUOTE},
    {Config::Input::KeyboardKey::LeftParen, SDLK_LEFTPAREN},
    {Config::Input::KeyboardKey::RightParen, SDLK_RIGHTPAREN},
    {Config::Input::KeyboardKey::Asterisk, SDLK_ASTERISK},
    {Config::Input::KeyboardKey::Plus, SDLK_PLUS},
    {Config::Input::KeyboardKey::Comma, SDLK_COMMA},
    {Config::Input::KeyboardKey::Minus, SDLK_MINUS},
    {Config::Input::KeyboardKey::Period, SDLK_PERIOD},
    {Config::Input::KeyboardKey::Slash, SDLK_SLASH},
    {Config::Input::KeyboardKey::Zero, SDLK_0},
    {Config::Input::KeyboardKey::One, SDLK_1},
    {Config::Input::KeyboardKey::Two, SDLK_2},
    {Config::Input::KeyboardKey::Three, SDLK_3},
    {Config::Input::KeyboardKey::Four, SDLK_4},
    {Config::Input::KeyboardKey::Five, SDLK_5},
    {Config::Input::KeyboardKey::Six, SDLK_6},
    {Config::Input::KeyboardKey::Seven, SDLK_7},
    {Config::Input::KeyboardKey::Eight, SDLK_8},
    {Config::Input::KeyboardKey::Nine, SDLK_9},
    {Config::Input::KeyboardKey::Colon, SDLK_COLON},
    {Config::Input::KeyboardKey::Semicolon, SDLK_SEMICOLON},
    {Config::Input::KeyboardKey::Less, SDLK_LESS},
    {Config::Input::KeyboardKey::Equals, SDLK_EQUALS},
    {Config::Input::KeyboardKey::Greater, SDLK_GREATER},
    {Config::Input::KeyboardKey::Question, SDLK_QUESTION},
    {Config::Input::KeyboardKey::At, SDLK_AT},
    {Config::Input::KeyboardKey::LeftBracket, SDLK_LEFTBRACKET},
    {Config::Input::KeyboardKey::Backslash, SDLK_BACKSLASH},
    {Config::Input::KeyboardKey::RightBracket, SDLK_RIGHTBRACKET},
    {Config::Input::KeyboardKey::Caret, SDLK_CARET},
    {Config::Input::KeyboardKey::Underscore, SDLK_UNDERSCORE},
    {Config::Input::KeyboardKey::Backquote, SDLK_BACKQUOTE},
    {Config::Input::KeyboardKey::A, SDLK_a},
    {Config::Input::KeyboardKey::B, SDLK_b},
    {Config::Input::KeyboardKey::C, SDLK_c},
    {Config::Input::KeyboardKey::D, SDLK_d},
    {Config::Input::KeyboardKey::E, SDLK_e},
    {Config::Input::KeyboardKey::F, SDLK_f},
    {Config::Input::KeyboardKey::G, SDLK_g},
    {Config::Input::KeyboardKey::H, SDLK_h},
    {Config::Input::KeyboardKey::I, SDLK_i},
    {Config::Input::KeyboardKey::J, SDLK_j},
    {Config::Input::KeyboardKey::K, SDLK_k},
    {Config::Input::KeyboardKey::L, SDLK_l},
    {Config::Input::KeyboardKey::M, SDLK_m},
    {Config::Input::KeyboardKey::N, SDLK_n},
    {Config::Input::KeyboardKey::O, SDLK_o},
    {Config::Input::KeyboardKey::P, SDLK_p},
    {Config::Input::KeyboardKey::Q, SDLK_q},
    {Config::Input::KeyboardKey::R, SDLK_r},
    {Config::Input::KeyboardKey::S, SDLK_s},
    {Config::Input::KeyboardKey::T, SDLK_t},
    {Config::Input::KeyboardKey::U, SDLK_u},
    {Config::Input::KeyboardKey::V, SDLK_v},
    {Config::Input::KeyboardKey::W, SDLK_w},
    {Config::Input::KeyboardKey::X, SDLK_x},
    {Config::Input::KeyboardKey::Y, SDLK_y},
    {Config::Input::KeyboardKey::Z, SDLK_z},
    {Config::Input::KeyboardKey::CapsLock, SDLK_CAPSLOCK},
    {Config::Input::KeyboardKey::F1, SDLK_F1},
    {Config::Input::KeyboardKey::F2, SDLK_F2},
    {Config::Input::KeyboardKey::F3, SDLK_F3},
    {Config::Input::KeyboardKey::F4, SDLK_F4},
    {Config::Input::KeyboardKey::F5, SDLK_F5},
    {Config::Input::KeyboardKey::F6, SDLK_F6},
    {Config::Input::KeyboardKey::F7, SDLK_F7},
    {Config::Input::KeyboardKey::F8, SDLK_F8},
    {Config::Input::KeyboardKey::F9, SDLK_F9},
    {Config::Input::KeyboardKey::F10, SDLK_F10},
    {Config::Input::KeyboardKey::F11, SDLK_F11},
    {Config::Input::KeyboardKey::F12, SDLK_F12},
    {Config::Input::KeyboardKey::Print, SDLK_PRINTSCREEN},
    {Config::Input::KeyboardKey::ScrollLock, SDLK_SCROLLLOCK},
    {Config::Input::KeyboardKey::Pause, SDLK_PAUSE},
    {Config::Input::KeyboardKey::Insert, SDLK_INSERT},
    {Config::Input::KeyboardKey::Home, SDLK_HOME},
    {Config::Input::KeyboardKey::PageUp, SDLK_PAGEUP},
    {Config::Input::KeyboardKey::Delete, SDLK_DELETE},
    {Config::Input::KeyboardKey::End, SDLK_END},
    {Config::Input::KeyboardKey::PageDown, SDLK_PAGEDOWN},
    {Config::Input::KeyboardKey::Right, SDLK_RIGHT},
    {Config::Input::KeyboardKey::Left, SDLK_LEFT},
    {Config::Input::KeyboardKey::Down, SDLK_DOWN},
    {Config::Input::KeyboardKey::Up, SDLK_UP},
};

static std::filesystem::path PREFERENCES_FILE = std::filesystem::temp_directory_path() / "docboy.prefs";

static void savePreferencesToCache(const Window& window) {
    const auto [x, y] = window.getPosition();
    INI ini;
    ini["window"] = {
        {"x", std::to_string(x)},
        {"y", std::to_string(y)},
    };
    ini_write(PREFERENCES_FILE, ini);
};

static INI loadPreferencesFromCache() {
    return ini_read(PREFERENCES_FILE);
};

int main(int argc, char** argv) {
    struct {
        std::string rom;
        std::string boot_rom;
        std::string config;
#ifdef ENABLE_DEBUGGER
        bool debugger {};
#endif
#ifdef ENABLE_PROFILER
        std::optional<uint64_t> profiler_ticks {};
#endif
        bool serial_console {};
        float scaling {1};
        double speed_up {1};
        bool dump_cartridge_info {};
    } args;

    auto parser = argumentum::argument_parser();
    auto params = parser.params();
    params.add_parameter(args.rom, "rom").help("ROM");
    params.add_parameter(args.boot_rom, "--boot-rom", "-b").nargs(1).help("Boot ROM");
    params.add_parameter(args.config, "--config", "-c").nargs(1).help("Config");
#ifdef ENABLE_DEBUGGER
    params.add_parameter(args.debugger, "--debugger", "-d").help("Attach CLI debugger");
#endif
#ifdef ENABLE_PROFILER
    params.add_parameter(args.profiler_ticks, "--profiler", "-p")
        .nargs(1)
        .default_value(1)
        .help("Run the profiler for the given ticks");
#endif
    params.add_parameter(args.serial_console, "--serial", "-s").help("Display serial output");
    params.add_parameter(args.scaling, "--scaling", "-z").nargs(1).default_value(1).help("Scaling factor");
    params.add_parameter(args.speed_up, "--speed-up", "-x").nargs(1).default_value(1).help("Speed up factor");
    params.add_parameter(args.dump_cartridge_info, "--cartridge-info", "-i").help("Dump cartridge info and quit");

    if (!parser.parse_args(argc, argv, 1))
        return 1;

    std::filesystem::path romPath = std::filesystem::path(args.rom);

    Config cfg;
    if (!args.config.empty()) {
        bool ok;
        std::string err;
        cfg = ConfigParser().parse(args.config, &ok, &err);
        if (!ok) {
            std::cerr << "ERROR: failed to parse config: '" << args.config << "': " << err << std::endl;
            return 1;
        }
    } else {
        cfg = Config::makeDefault();
    }

    std::unique_ptr<ICartridge> cartridge;
    try {
        cartridge = CartridgeFactory::makeCartridge(args.rom);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: failed to load rom: '" << args.rom << "' " << e.what() << std::endl;
        return 1;
    }

    if (args.dump_cartridge_info) {
        dump_cartridge_info(*cartridge);
        return 0;
    }

    std::unique_ptr<IBootROM> bootRom;
    if (!args.boot_rom.empty()) {
        bootRom = BootROMFactory::makeBootROM(args.boot_rom);
        if (!bootRom) {
            std::cerr << "ERROR: failed to load boot rom: '" << args.boot_rom << "'" << std::endl;
            return 1;
        }
    }

    auto frequency = (uint64_t)(args.speed_up * Specs::FREQUENCY);

#ifdef ENABLE_DEBUGGER
    class DebuggerFrontendCliObserver : public DebuggerFrontendCli::Observer {
    public:
        explicit DebuggerFrontendCliObserver() :
            window() {
        }
        void setWindow(Window* w) {
            window = w;
        }
        void onReadCommand() override {
            // render the current frame before let the frontend stuck on command input
            if (window)
                window->render();
        }

    private:
        Window* window;
    };

    DebuggableGameBoy gb = DebuggableGameBoy::Builder().setBootROM(std::move(bootRom)).setFrequency(frequency).build();
    DebuggableCore core(gb);
    std::unique_ptr<DebuggerBackend> debuggerBackend;
    std::unique_ptr<DebuggerFrontendCli> debuggerFrontend;
    std::unique_ptr<DebuggerFrontendCliObserver> debuggerFrontendObserver;

    if (args.debugger) {
        debuggerBackend = std::make_unique<DebuggerBackend>(core);
        debuggerFrontend = std::make_unique<DebuggerFrontendCli>(*debuggerBackend);

        DebuggerFrontendCli::Config debuggerCfg = DebuggerFrontendCli::Config::makeDefault();
        debuggerCfg.sections.breakpoints = cfg.debug.sections.breakpoints;
        debuggerCfg.sections.watchpoints = cfg.debug.sections.watchpoints;
        debuggerCfg.sections.cpu = cfg.debug.sections.cpu;
        debuggerCfg.sections.ppu = cfg.debug.sections.ppu;
        debuggerCfg.sections.flags = cfg.debug.sections.flags;
        debuggerCfg.sections.registers = cfg.debug.sections.registers;
        debuggerCfg.sections.interrupts = cfg.debug.sections.interrupts;
        debuggerCfg.sections.io.joypad = cfg.debug.sections.io.joypad;
        debuggerCfg.sections.io.serial = cfg.debug.sections.io.serial;
        debuggerCfg.sections.io.timers = cfg.debug.sections.io.timers;
        debuggerCfg.sections.io.sound = cfg.debug.sections.io.sound;
        debuggerCfg.sections.io.lcd = cfg.debug.sections.io.lcd;
        debuggerCfg.sections.code = cfg.debug.sections.code;
        debuggerFrontend->setConfig(debuggerCfg);

        debuggerFrontendObserver = std::make_unique<DebuggerFrontendCliObserver>();
        debuggerFrontend->setObserver(debuggerFrontendObserver.get());
    }
#else
    GameBoy gb = GameBoy::Builder().setBootROM(std::move(bootRom)).setFrequency(frequency).build();
    Core core(gb);
#endif

#ifdef ENABLE_PROFILER
    auto printProfilerResult = [](const ProfilerResult& result) {
        auto millis = duration_cast<std::chrono::milliseconds>(result.executionTime).count();
        auto seconds = (double)millis / 1000;
        auto effectiveFrequency = (uint64_t)((double)result.ticks / seconds);
        auto effectiveSpeedUp = (double)effectiveFrequency / Specs::FREQUENCY;
        std::cout << "====== PROFILING RESULT ======\n";
        std::cout << "Ticks                : " << result.ticks << "\n";
        std::cout << "Time (s)             : " << seconds << "\n";
        std::cout << "EffectiveFrequency   : " << effectiveFrequency << "\n";
        std::cout << "EffectiveSpeedUp     : " << effectiveSpeedUp << "\n";
    };

    Profiler profiler(core, gb);
    if (args.profiler_ticks) {
        profiler.setMaxTicks(*args.profiler_ticks);
    }
#endif

    int winX = Window::WINDOW_POSITION_UNDEFINED;
    int winY = Window::WINDOW_POSITION_UNDEFINED;

    INI prefs = loadPreferencesFromCache();
    if (const auto windowSectionIt = prefs.find("window"); windowSectionIt != prefs.end()) {
        const auto& windowSection = windowSectionIt->second;
        if (const auto& xIt = windowSection.find("x"); xIt != windowSection.end())
            winX = std::stoi(xIt->second);
        if (const auto& yIt = windowSection.find("y"); yIt != windowSection.end())
            winY = std::stoi(yIt->second);
    }

    FrameBufferLCD& lcd = gb.lcd;
    Window window(gb.lcd.getFrameBuffer(), gb.lcdController, winX, winY, args.scaling);

    std::shared_ptr<SerialLink> serialLink;
    std::unique_ptr<ISerialEndpoint> serialConsole;

    if (args.serial_console) {
        serialConsole = std::make_unique<SerialConsole>(std::cerr);
        serialLink = std::make_shared<SerialLink>();
        serialLink->plug1.attach(serialConsole.get());
        core.attachSerialLink(serialLink->plug2);
    }

    core.loadROM(std::move(cartridge));

    static const std::set<SDL_Keycode> RESERVED_SDL_KEYS = {SDLK_F1, SDLK_F2, SDLK_F11, SDLK_F12, SDLK_f};

    std::map<SDL_Keycode, IJoypad::Key> keyboardInputMapping;
    for (const auto& [joypadKey, keyboardKey] : cfg.input.keyboardMapping) {
        SDL_Keycode sdlKey = KEYBOARD_KEYS_TO_SDL_KEYS[keyboardKey];
        if (!RESERVED_SDL_KEYS.contains(sdlKey))
            keyboardInputMapping[sdlKey] = joypadKey;
    }

    SDL_Event e;

    static constexpr uint64_t OVERLAY_TEXT_GUID = 1;
    static constexpr uint64_t FPS_TEXT_GUID = 2;

    auto drawOverlayText = [&window](const std::string& str) {
        window.addText(str, 4, 4, 0xFFFFFFFF, 120, OVERLAY_TEXT_GUID);
    };

    auto drawFPS = [&window](uint32_t fps) {
        std::string fpsString = std::to_string(fps);
        window.addText(fpsString,
                       static_cast<int>(Window::WINDOW_WIDTH - 4 - fpsString.size() * Window::TEXT_LETTER_WIDTH), 4,
                       0xFFFFFFFF, Window::TEXT_DURATION_PERSISTENT, FPS_TEXT_GUID);
    };

    auto undrawFPS = [&window] {
        window.removeText(FPS_TEXT_GUID);
    };

    bool quit = false;

    bool showFps = false;
    uint32_t fps = 0;
    std::chrono::high_resolution_clock::time_point lastFpsSampling = std::chrono::high_resolution_clock::now();

    while (!quit && core.isOn()) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                case SDLK_F1: {
                    State state;
                    core.saveState(state);
                    if (save_state(state, romPath.replace_extension("sav")))
                        drawOverlayText("State saved");
                    break;
                }
                case SDLK_F2: {
                    if (std::optional<State> optionalState = load_state(romPath.replace_extension("sav"))) {
                        drawOverlayText("State loaded");
                        core.loadState(*optionalState);
                    }
                    break;
                }
                case SDLK_F11:
                    if (screenshot_dat(lcd.getFrameBuffer(), romPath.replace_extension("dat")))
                        drawOverlayText("Screenshot saved");
                    break;
                case SDLK_F12:
                    if (screenshot_bmp(lcd.getFrameBuffer(), romPath.replace_extension("bmp")))
                        drawOverlayText("Screenshot saved");
                    break;
                case SDLK_f:
                    showFps = !showFps;
                    if (showFps) {
                        lastFpsSampling = std::chrono::high_resolution_clock::now();
                        fps = 0;
                    } else {
                        undrawFPS();
                    }
                default:
                    if (auto mapping = keyboardInputMapping.find(e.key.keysym.sym);
                        mapping != keyboardInputMapping.end()) {
                        core.setKey(mapping->second, IJoypad::Pressed);
                    }
                }
                break;
            case SDL_KEYUP:
                if (auto mapping = keyboardInputMapping.find(e.key.keysym.sym); mapping != keyboardInputMapping.end()) {
                    core.setKey(mapping->second, IJoypad::Released);
                }
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    savePreferencesToCache(window);
                }
            }
        }

#ifdef ENABLE_PROFILER
        // Update emulator until next frame through the profiler
        profiler.frame();
        if (profiler.isProfilingFinished()) {
            printProfilerResult(profiler.getProfilerResult());
            quit = true;
        }
#else
        // Update emulator until next frame
        core.frame();
#endif

        // Eventually update FPS estimation
        fps++;
        if (showFps) {
            const auto now = std::chrono::high_resolution_clock::now();
            if (now - lastFpsSampling > std::chrono::milliseconds(1000)) {
                drawFPS(fps);
                lastFpsSampling = now;
                fps = 0;
            }
        }

        // Render frame
        window.render();
    }

    return 0;
}
