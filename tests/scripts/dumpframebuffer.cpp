#include "testutils/img.h"
#include <iostream>

// usage: dumpframebuffer <in_file>
int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: dumpframebuffer <in_file>" << std::endl;
        exit(1);
    }

    dump_png_framebuffer(argv[1]);

    return 0;
}