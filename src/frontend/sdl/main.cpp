#include <SDL.h>
#include <iostream>
#include <filesystem>
#include "argparser.h"
#include "core/gameboy.h"
#include "core/core.h"
#include "core/boot/bootromfactory.h"
#include "core/serial/endpoints/console.h"
#include "helpers.h"
#include "window.h"
#include "utils/fileutils.h"
#include "core/config/config.h"
#include "core/config/parser.h"

#ifdef ENABLE_DEBUGGER
#include "core/debugger/gameboy.h"
#include "core/debugger/core/core.h"
#include "core/debugger/backend.h"
#include "core/debugger/frontendcli.h"
#endif

static void screenshot_bmp(uint32_t *framebuffer) {
    int i = 0;
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path p;
    do {
        p = cwd / ("screenshot" + std::to_string(i) + ".bmp");
        i++;
    } while (std::filesystem::exists(p));

    std::string path = absolute(p).c_str();

    if (screenshot(framebuffer,
                   Specs::Display::WIDTH, Specs::Display::HEIGHT,
                   SDL_PIXELFORMAT_RGBA8888, path))
        std::cout << "Screenshot saved to: " << path << std::endl;
}

static void screenshot_dat(uint32_t *framebuffer) {
    int i = 0;
    std::filesystem::path cwd = std::filesystem::current_path();
    std::filesystem::path p;
    do {
        p = cwd / ("screenshot" + std::to_string(i) + ".dat");
        i++;
    } while (std::filesystem::exists(p));

    std::string path = absolute(p).c_str();

    if (write_file(path,
                   framebuffer,
                   sizeof(uint32_t) * Specs::Display::WIDTH * Specs::Display::HEIGHT))
        std::cout << "Screenshot saved to: " << path << std::endl;
}

static std::map<Config::Input::KeyboardKey, SDL_Keycode> KEYBOARD_KEYS_TO_SDL_KEYS = {
    { Config::Input::KeyboardKey::Return, SDLK_RETURN },
    { Config::Input::KeyboardKey::Escape, SDLK_ESCAPE },
    { Config::Input::KeyboardKey::Backspace, SDLK_BACKSPACE },
    { Config::Input::KeyboardKey::Tab, SDLK_TAB },
    { Config::Input::KeyboardKey::Space, SDLK_SPACE },
    { Config::Input::KeyboardKey::Exclaim, SDLK_EXCLAIM },
    { Config::Input::KeyboardKey::DoubleQuote, SDLK_QUOTEDBL },
    { Config::Input::KeyboardKey::Hash, SDLK_HASH },
    { Config::Input::KeyboardKey::Percent, SDLK_PERCENT },
    { Config::Input::KeyboardKey::Dollar, SDLK_DOLLAR },
    { Config::Input::KeyboardKey::Ampersand, SDLK_AMPERSAND },
    { Config::Input::KeyboardKey::Quote, SDLK_QUOTE },
    { Config::Input::KeyboardKey::LeftParen, SDLK_LEFTPAREN },
    { Config::Input::KeyboardKey::RightParen, SDLK_RIGHTPAREN },
    { Config::Input::KeyboardKey::Asterisk, SDLK_ASTERISK },
    { Config::Input::KeyboardKey::Plus, SDLK_PLUS },
    { Config::Input::KeyboardKey::Comma, SDLK_COMMA },
    { Config::Input::KeyboardKey::Minus, SDLK_MINUS },
    { Config::Input::KeyboardKey::Period, SDLK_PERIOD },
    { Config::Input::KeyboardKey::Slash, SDLK_SLASH },
    { Config::Input::KeyboardKey::Zero, SDLK_0 },
    { Config::Input::KeyboardKey::One, SDLK_1 },
    { Config::Input::KeyboardKey::Two, SDLK_2 },
    { Config::Input::KeyboardKey::Three, SDLK_3 },
    { Config::Input::KeyboardKey::Four, SDLK_4 },
    { Config::Input::KeyboardKey::Five, SDLK_5 },
    { Config::Input::KeyboardKey::Six, SDLK_6 },
    { Config::Input::KeyboardKey::Seven, SDLK_7 },
    { Config::Input::KeyboardKey::Eight, SDLK_8 },
    { Config::Input::KeyboardKey::Nine, SDLK_9 },
    { Config::Input::KeyboardKey::Colon, SDLK_COLON },
    { Config::Input::KeyboardKey::Semicolon, SDLK_SEMICOLON },
    { Config::Input::KeyboardKey::Less, SDLK_LESS },
    { Config::Input::KeyboardKey::Equals, SDLK_EQUALS },
    { Config::Input::KeyboardKey::Greater, SDLK_GREATER },
    { Config::Input::KeyboardKey::Question, SDLK_QUESTION },
    { Config::Input::KeyboardKey::At, SDLK_AT },
    { Config::Input::KeyboardKey::LeftBracket, SDLK_LEFTBRACKET },
    { Config::Input::KeyboardKey::Backslash, SDLK_BACKSLASH },
    { Config::Input::KeyboardKey::RightBracket, SDLK_RIGHTBRACKET },
    { Config::Input::KeyboardKey::Caret, SDLK_CARET },
    { Config::Input::KeyboardKey::Underscore, SDLK_UNDERSCORE },
    { Config::Input::KeyboardKey::Backquote, SDLK_BACKQUOTE },
    { Config::Input::KeyboardKey::A, SDLK_a },
    { Config::Input::KeyboardKey::B, SDLK_b },
    { Config::Input::KeyboardKey::C, SDLK_c },
    { Config::Input::KeyboardKey::D, SDLK_d },
    { Config::Input::KeyboardKey::E, SDLK_e },
    { Config::Input::KeyboardKey::F, SDLK_f },
    { Config::Input::KeyboardKey::G, SDLK_g },
    { Config::Input::KeyboardKey::H, SDLK_h },
    { Config::Input::KeyboardKey::I, SDLK_i },
    { Config::Input::KeyboardKey::J, SDLK_j },
    { Config::Input::KeyboardKey::K, SDLK_k },
    { Config::Input::KeyboardKey::L, SDLK_l },
    { Config::Input::KeyboardKey::M, SDLK_m },
    { Config::Input::KeyboardKey::N, SDLK_n },
    { Config::Input::KeyboardKey::O, SDLK_o },
    { Config::Input::KeyboardKey::P, SDLK_p },
    { Config::Input::KeyboardKey::Q, SDLK_q },
    { Config::Input::KeyboardKey::R, SDLK_r },
    { Config::Input::KeyboardKey::S, SDLK_s },
    { Config::Input::KeyboardKey::T, SDLK_t },
    { Config::Input::KeyboardKey::U, SDLK_u },
    { Config::Input::KeyboardKey::V, SDLK_v },
    { Config::Input::KeyboardKey::W, SDLK_w },
    { Config::Input::KeyboardKey::X, SDLK_x },
    { Config::Input::KeyboardKey::Y, SDLK_y },
    { Config::Input::KeyboardKey::Z, SDLK_z },
    { Config::Input::KeyboardKey::CapsLock, SDLK_CAPSLOCK },
    { Config::Input::KeyboardKey::F1, SDLK_F1 },
    { Config::Input::KeyboardKey::F2, SDLK_F2 },
    { Config::Input::KeyboardKey::F3, SDLK_F3 },
    { Config::Input::KeyboardKey::F4, SDLK_F4 },
    { Config::Input::KeyboardKey::F5, SDLK_F5 },
    { Config::Input::KeyboardKey::F6, SDLK_F6 },
    { Config::Input::KeyboardKey::F7, SDLK_F7 },
    { Config::Input::KeyboardKey::F8, SDLK_F8 },
    { Config::Input::KeyboardKey::F9, SDLK_F9 },
    { Config::Input::KeyboardKey::F10, SDLK_F10 },
    { Config::Input::KeyboardKey::F11, SDLK_F11 },
    { Config::Input::KeyboardKey::F12, SDLK_F12 },
    { Config::Input::KeyboardKey::Print, SDLK_PRINTSCREEN },
    { Config::Input::KeyboardKey::ScrollLock, SDLK_SCROLLLOCK },
    { Config::Input::KeyboardKey::Pause, SDLK_PAUSE },
    { Config::Input::KeyboardKey::Insert, SDLK_INSERT },
    { Config::Input::KeyboardKey::Home, SDLK_HOME },
    { Config::Input::KeyboardKey::PageUp, SDLK_PAGEUP },
    { Config::Input::KeyboardKey::Delete, SDLK_DELETE },
    { Config::Input::KeyboardKey::End, SDLK_END },
    { Config::Input::KeyboardKey::PageDown, SDLK_PAGEDOWN },
    { Config::Input::KeyboardKey::Right, SDLK_RIGHT },
    { Config::Input::KeyboardKey::Left, SDLK_LEFT },
    { Config::Input::KeyboardKey::Down, SDLK_DOWN },
    { Config::Input::KeyboardKey::Up, SDLK_UP },
};


int main(int argc, char **argv) {
    struct {
        std::string rom;
        std::string boot_rom;
        std::string config;
        bool debugger {};
        bool serial_console {};
        float scaling {1};
        double speed_up {1};
    } args;

    auto parser = argumentum::argument_parser();
    auto params = parser.params();
    params
        .add_parameter(args.rom, "rom")
        .help("ROM");
    params
        .add_parameter(args.boot_rom, "--boot-rom", "-b")
        .nargs(1)
        .help("Boot ROM");
    params
        .add_parameter(args.config, "--config", "-c")
        .nargs(1)
        .help("Config");
#ifdef ENABLE_DEBUGGER
    params
        .add_parameter(args.debugger, "--debugger", "-d")
        .help("Attach CLI debugger");
#endif
    params
        .add_parameter(args.serial_console, "--serial", "-s")
        .help("Display serial output");
    params
        .add_parameter(args.scaling, "--scaling", "-z")
        .nargs(1)
        .default_value(1)
        .help("Scaling factor");
    params
        .add_parameter(args.speed_up, "--speed-up", "-x")
        .nargs(1)
        .default_value(1)
        .help("Speed up factor");

    if (!parser.parse_args(argc, argv, 1))
        return 1;

    Config cfg;
    if (!args.config.empty()) {
        bool ok;
        std::string err;
        cfg = ConfigParser::parse(args.config, &ok, &err);
        if (!ok) {
            std::cerr << "ERROR: failed to parse config: '" << args.config << "'" << std::endl;
            std::cerr << "ERROR: " + err << std::endl;
            return 1;
        }
    } else {
        cfg = Config::makeDefault();
    }

    std::unique_ptr<IBootROM> bootRom;
    if (!args.boot_rom.empty()) {
        bootRom = BootROMFactory::makeBootROM(args.boot_rom);
        if (!bootRom) {
            std::cerr << "ERROR: failed to load boot rom: '" << args.boot_rom << "'" << std::endl;
            return 1;
        }
    }

#ifdef ENABLE_DEBUGGER
    class DebuggerFrontendCliObserver : public DebuggerFrontendCli::Observer {
    public:
        explicit DebuggerFrontendCliObserver() : window() {}
        void setWindow(Window *w) { window = w; }
        void onReadCommand() override {
            // render the current frame before let the frontend stuck on command input
            if (window)
                window->render();
        }
    private:
        Window *window;
    };

    DebuggableGameBoy gb(std::move(bootRom));
    DebuggableCore core(gb);
    std::unique_ptr<DebuggerBackend> debuggerBackend;
    std::unique_ptr<DebuggerFrontendCli> debuggerFrontend;
    std::unique_ptr<DebuggerFrontendCliObserver> debuggerFrontendObserver;

    if (args.debugger) {
        debuggerBackend = std::make_unique<DebuggerBackend>(core);
        debuggerFrontend = std::make_unique<DebuggerFrontendCli>(*debuggerBackend);
        debuggerFrontendObserver = std::make_unique<DebuggerFrontendCliObserver>();
        debuggerFrontend->setObserver(debuggerFrontendObserver.get());
    }
#else
    GameBoy gb(std::move(bootRom));
    Core core(gb);
#endif

    FrameBufferLCD &lcd = gb.lcd;
    Window window(gb.lcd.getFrameBuffer(), gb.lcdController, args.scaling);

    std::shared_ptr<SerialLink> serialLink;
    std::unique_ptr<ISerialEndpoint> serialConsole;

    if (args.serial_console) {
        serialConsole = std::make_unique<SerialConsole>(std::cerr);
        serialLink = std::make_shared<SerialLink>();
        serialLink->plug1.attach(serialConsole.get());
        core.attachSerialLink(serialLink->plug2);
    }

    if (!core.loadROM(args.rom)) {
        std::cerr << "ERROR: failed to load rom: '" << args.rom << "'" << std::endl;
        return 1;
    }

    static const std::set<SDL_Keycode> RESERVED_SDL_KEYS = { SDLK_F11, SDLK_F12 };

    std::map<SDL_Keycode, IJoypad::Key> keyboardInputMapping;
    for (const auto &[joypadKey, keyboardKey] : cfg.input.keyboardMapping) {
        SDL_Keycode sdlKey = KEYBOARD_KEYS_TO_SDL_KEYS[keyboardKey];
        if (!RESERVED_SDL_KEYS.contains(sdlKey))
            keyboardInputMapping[sdlKey] = joypadKey;
    }

    SDL_Event e;

    bool quit = false;
    while (!quit && core.isOn()) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            }
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_F11:
                        screenshot_dat(lcd.getFrameBuffer());
                        break;
                    case SDLK_F12:
                        screenshot_bmp(lcd.getFrameBuffer());
                        break;
                    default:
                        if (auto mapping = keyboardInputMapping.find(e.key.keysym.sym);
                                mapping != keyboardInputMapping.end()) {
                            core.setKey(mapping->second, IJoypad::Pressed);
                        }
                }
            } else if (e.type == SDL_KEYUP) {
                if (auto mapping = keyboardInputMapping.find(e.key.keysym.sym);
                        mapping != keyboardInputMapping.end()) {
                    core.setKey(mapping->second, IJoypad::Released);
                }
            }
        }

        // Update emulator until next frame
        core.frame();

        // Render frame
        window.render();
    }

    return 0;
}
