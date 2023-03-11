#include <SDL.h>
#include <iostream>
#include <filesystem>
#include "argparser.h"
#include "core/gameboy.h"
#include "core/core.h"
#include "core/boot/bootromfactory.h"
#include "core/serial/endpoints/console.h"
#include "core/lcd/framebufferlcd.h"
#include "helpers.h"
#include "utils/fileutils.h"
#include "window.h"

#ifdef ENABLE_DEBUGGER
#include "core/debugger/core/core.h"
#include "core/debugger/backend.h"
#include "core/debugger/frontend.h"
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

int main(int argc, char **argv) {
    struct {
        std::string rom;
        std::string boot_rom;
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

    std::shared_ptr<FrameBufferLCD> lcd = std::make_shared<FrameBufferLCD>();

    Window window(*lcd, args.scaling);

    std::unique_ptr<Impl::IBootROM> bootRom;
    if (!args.boot_rom.empty())
        bootRom = BootROMFactory::makeBootROM(args.boot_rom);

    GameBoy gb = GameBoy::Builder()
            .setFrequency(static_cast<uint64_t >(args.speed_up * Specs::FREQUENCY))
            .setBootROM(std::move(bootRom))
            .setLCD(lcd)
            .build();

#ifdef ENABLE_DEBUGGER
    DebuggableCore core(gb);
    std::unique_ptr<DebuggerBackend> debuggerBackend;
    std::unique_ptr<DebuggerFrontendCli> debuggerFrontend;

    class DebuggerFrontendCliObserver : public DebuggerFrontendCli::Observer {
    public:
        explicit DebuggerFrontendCliObserver(Window *w) : window(w) {}
        void onReadCommand() override {
            // render the current frame before let the frontend stuck on command input
            window->render();
        }
    private:
        Window *window;
    };

    DebuggerFrontendCliObserver frontendObserver(&window);

    if (args.debugger) {
        debuggerBackend = std::make_unique<DebuggerBackend>(core);
        debuggerFrontend = std::make_unique<DebuggerFrontendCli>(*debuggerBackend);
        debuggerFrontend->setObserver(&frontendObserver);
    }
#else
    Core core(gb);
#endif

    std::shared_ptr<SerialLink> serialLink;
    std::unique_ptr<SerialEndpoint> serialConsole;

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
                        screenshot_dat(lcd->getFrameBuffer());
                        break;
                    case SDLK_F12:
                        screenshot_bmp(lcd->getFrameBuffer());
                        break;
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
