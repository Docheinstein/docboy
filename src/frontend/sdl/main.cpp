#include <iostream>
#include "utils/log.h"
#include "core/core.h"
#include "utils/binutils.h"
#include "argparser.h"
#include "window.h"
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

    Core core;
//    Window w;
//    w.show();

    core.loadROM(args.rom);
    core.start();
    return 0;
}
