#include <iostream>
#include "log/log.h"
#include "core/gible.h"
#include "utils/binutils.h"
#include "argparser.h"
#include "debugger/debuggerfrontendcli.h"
#include <bitset>

int main(int argc, char **argv) {
    struct {
        std::string rom;
        bool debugger;
        bool serialConsole;
    } args;

    auto parser = argumentum::argument_parser();
    auto params = parser.params();
    params
        .add_parameter(args.rom, "rom")
        .help("ROM");
    params
        .add_parameter(args.debugger, "--debugger", "-d")
        .help("Attach CLI debugger");
    params
        .add_parameter(args.serialConsole, "--serial", "-s")
        .help("Display serial output");
    if (!parser.parse_args(argc, argv, 1))
        return 1;

    Gible gible;

    std::unique_ptr<DebuggerFrontend> cliDebugger;

    if (args.debugger) {
        cliDebugger = std::make_unique<DebuggerFrontendCli>(gible);
        gible.attachDebugger(&*cliDebugger);
    }

    std::unique_ptr<SerialEndpoint> serialConsole;

    if (args.serialConsole) {
        serialConsole = std::make_unique<SerialConsoleEndpoint>(std::cout);
        SerialLink serialLink(&gible, &*serialConsole);
        gible.attachSerialLink(&serialLink);
    }

    gible.loadROM(args.rom);
    gible.start();
    return 0;
}
