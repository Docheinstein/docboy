#include <iostream>
#include "argparser.h"
#include "core/core.h"
#include "core/boot/bootromfactory.h"
#include "core/gameboybuilder.h"
#include "core/debugger/debuggablecore.h"
#include "core/debugger/debuggerbackend.h"
#include "core/debugger/debuggerfrontendcli.h"
#include "core/serial/serialconsole.h"
#include "noguidisplay.h"

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

    std::unique_ptr<IBootROM> bootRom;
    if (!args.boot_rom.empty())
        bootRom = BootROMFactory::makeBootROM(args.boot_rom);

    GameBoy gb = GameBoyBuilder()
            .setBootROM(std::move(bootRom))
            .setDisplay(std::make_unique<NoGuiDisplay>())
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

    core.start();
    return 0;
}
