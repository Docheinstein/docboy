#include <iostream>
#include "argparser.h"
#include "core/core.h"
#include "core/debugger/debuggablecore.h"
#include "core/debugger/debuggerbackend.h"
#include "core/debugger/debuggerfrontendcli.h"
#include "core/serial/serialconsole.h"

int main(int argc, char **argv) {
    struct {
        std::string rom;
        bool debugger {};
        bool serialConsole {};
    } args;

    auto parser = argumentum::argument_parser();
    auto params = parser.params();
    params
        .add_parameter(args.rom, "rom")
        .help("ROM");
#ifdef ENABLE_DEBUGGER
    params
        .add_parameter(args.debugger, "--debugger", "-d")
        .help("Attach CLI debugger");
#endif
    params
        .add_parameter(args.serialConsole, "--serial", "-s")
        .help("Display serial output");
    if (!parser.parse_args(argc, argv, 1))
        return 1;

#ifdef ENABLE_DEBUGGER
    DebuggableCore core;
    std::unique_ptr<DebuggerBackend> debuggerBackend;
    std::unique_ptr<IDebuggerFrontend> debuggerFrontend;

    if (args.debugger) {
        debuggerBackend = std::make_unique<DebuggerBackend>(core);
        debuggerFrontend = std::make_unique<DebuggerFrontendCli>(*debuggerBackend);
    }
#else
    Core core;
#endif

    std::shared_ptr<SerialLink> serialLink;
    std::unique_ptr<SerialEndpoint> serialConsole;

    if (args.serialConsole) {
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
