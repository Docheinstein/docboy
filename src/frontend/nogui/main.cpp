#include <iostream>
#include "argparser.h"
#include "core/core.h"
#include "core/boot/bootromfactory.h"
#include "core/gameboy.h"
#include "core/serial/endpoints/console.h"
#include "noguilcd.h"

#ifdef ENABLE_DEBUGGER
#include "core/debugger/core/core.h"
#include "core/debugger/backend.h"
#include "core/debugger/frontendcli.h"
#endif

int main(int argc, char **argv) {
    struct {
        std::string rom;
        std::string boot_rom;
        bool debugger {};
        bool serial_console {};
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
    if (!parser.parse_args(argc, argv, 1))
        return 1;

    std::unique_ptr<Impl::IBootROM> bootRom;
    if (!args.boot_rom.empty())
        bootRom = BootROMFactory::makeBootROM(args.boot_rom);

    GameBoy gb = GameBoy::Builder()
            .setBootROM(std::move(bootRom))
            .setLCD(std::make_unique<NoGuiLCD>())
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
        serialLink = std::make_shared<SerialLink>();
        serialLink->plug1.attach(serialConsole.get());
        core.attachSerialLink(serialLink->plug2);
    }

    if (!core.loadROM(args.rom)) {
        std::cerr << "ERROR: failed to load rom: '" << args.rom << "'" << std::endl;
        return 1;
    }

    while (core.isOn()) {
        core.tick();
    }

    return 0;
}
