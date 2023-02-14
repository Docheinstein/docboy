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
    } args;

    auto parser = argumentum::argument_parser();
    auto params = parser.params();
    params
        .add_parameter(args.rom, "rom")
        .help("ROM");
    if (!parser.parse_args(argc, argv, 1))
        return 1;

    Gible gible;
    DebuggerFrontendCli cliDebugger(gible);
    gible.attachDebugger(cliDebugger);
    gible.loadROM(args.rom);
    gible.start();
    return 0;
}
