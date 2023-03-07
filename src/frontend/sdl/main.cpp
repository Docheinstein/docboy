#include <SDL.h>
#include <iostream>
#include "argparser.h"
#include "core/core.h"
#include "core/boot/bootromfactory.h"
#include "core/gameboybuilder.h"
#include "core/debugger/debuggablecore.h"
#include "core/debugger/debuggerbackend.h"
#include "core/debugger/debuggerfrontendcli.h"
#include "core/serial/serialconsole.h"
#include "window.h"

int main(int argc, char **argv) {
    struct {
        std::string rom;
        std::string boot_rom;
        bool debugger {};
        bool serial_console {};
        float scaling {1};
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

    if (!parser.parse_args(argc, argv, 1))
        return 1;

    std::unique_ptr<SDLLCD> lcd = std::make_unique<SDLLCD>();
    Window window(*lcd, args.scaling);

    std::unique_ptr<IBootROM> bootRom;
    if (!args.boot_rom.empty())
        bootRom = BootROMFactory::makeBootROM(args.boot_rom);

    GameBoy gb = GameBoyBuilder()
            .setBootROM(std::move(bootRom))
            .setLCD(std::move(lcd))
            .build();

#ifdef ENABLE_DEBUGGER
    DebuggableCore core(gb);
    std::unique_ptr<DebuggerBackend> debuggerBackend;
    std::unique_ptr<IDebuggerFrontend> debuggerFrontend;

    if (args.debugger) {
        debuggerBackend = std::make_unique<DebuggerBackend>(core);
        debuggerFrontend = std::make_unique<DebuggerFrontendCli>(*debuggerBackend);
    }
#else
    Core core(gb);
#endif

    std::shared_ptr<SerialLink> serialLink;
    std::unique_ptr<SerialEndpoint> serialConsole;

    if (args.serial_console) {
        serialConsole = std::make_unique<SerialConsole>(std::cerr);
        serialLink = std::make_shared<SerialLink>(&core, serialConsole.get());
        core.attachSerialLink(serialLink);
    }

    if (!core.loadROM(args.rom)) {
        std::cerr << "ERROR: failed to load rom: '" << args.rom << "'" << std::endl;
        return 1;
    }

    SDL_Event e;

    auto now = std::chrono::high_resolution_clock::now();
    auto nextTick = now + std::chrono::milliseconds(16);

    bool quit = false;
    while (!quit) {
        do {
            if (!core.tick())
                quit = true;
            now = std::chrono::high_resolution_clock::now();
        } while (now < nextTick && !quit);
        nextTick = now + std::chrono::milliseconds(16);

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT)
                quit = true;
        }

        window.render();
    }


    return 0;
}
