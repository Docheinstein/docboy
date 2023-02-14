#include <iostream>
#include "log/log.h"
#include "core/gible.h"
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

    Gible gible;
//    Window w;
//    w.show();

    gible.loadROM(args.rom);
    gible.start();
    return 0;
}
