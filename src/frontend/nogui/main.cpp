#include "argparser.h"
#include "core/boot/bootromfactory.h"
#include "core/core.h"
#include "core/gameboy.h"
#include "core/serial/endpoints/console.h"
#include <iostream>

#ifdef ENABLE_DEBUGGER
#include "core/debugger/backend.h"
#include "core/debugger/core/core.h"
#include "core/debugger/frontendcli.h"
#include "core/debugger/gameboy.h"
#endif

int main(int argc, char** argv) {
    struct {
        std::string rom;
        std::string boot_rom;
        bool debugger {};
        bool serial_console {};
        double speed_up {1};
    } args;

    auto parser = argumentum::argument_parser();
    auto params = parser.params();
    params.add_parameter(args.rom, "rom").help("ROM");
    params.add_parameter(args.boot_rom, "--boot-rom", "-b").nargs(1).help("Boot ROM");
#ifdef ENABLE_DEBUGGER
    params.add_parameter(args.debugger, "--debugger", "-d").help("Attach CLI debugger");
#endif
    params.add_parameter(args.serial_console, "--serial", "-s").help("Display serial output");
    params.add_parameter(args.speed_up, "--speed-up", "-x").nargs(1).default_value(1).help("Speed up factor");
    if (!parser.parse_args(argc, argv, 1))
        return 1;

    if (!parser.parse_args(argc, argv, 1))
        return 1;

    std::unique_ptr<IBootROM> bootRom;
    if (!args.boot_rom.empty()) {
        bootRom = BootROMFactory::makeBootROM(args.boot_rom);
        if (!bootRom) {
            std::cerr << "ERROR: failed to load boot rom: '" << args.boot_rom << "'" << std::endl;
            return 1;
        }
    }

#ifdef ENABLE_DEBUGGER
    DebuggableGameBoy gb(std::move(bootRom));
    DebuggableCore core(gb);
    std::unique_ptr<DebuggerBackend> debuggerBackend;
    std::unique_ptr<DebuggerFrontendCli> debuggerFrontend;

    if (args.debugger) {
        debuggerBackend = std::make_unique<DebuggerBackend>(core);
        debuggerFrontend = std::make_unique<DebuggerFrontendCli>(*debuggerBackend);
    }
#else
    GameBoy gb(std::move(bootRom));
    Core core(gb);
#endif

    std::shared_ptr<SerialLink> serialLink;
    std::unique_ptr<ISerialEndpoint> serialConsole;

    if (args.serial_console) {
        serialConsole = std::make_unique<SerialConsole>(std::cerr);
        serialLink = std::make_shared<SerialLink>();
        serialLink->plug1.attach(serialConsole.get());
        core.attachSerialLink(serialLink->plug2);
    }

    __try {
        core.loadROM(args.rom);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: failed to load rom: '" << args.rom << "' " << e.what() << std::endl;
        return 1;
    }

    while (core.isOn()) {
        core.tick();
    }
}
