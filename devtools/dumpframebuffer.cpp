#include "testutils/framebuffers.h"
#include <iostream>

// usage: dumpframebuffer <in_file>
int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: dumpframebuffer <in_file>" << std::endl;
        exit(1);
    }

    dump_framebuffer_png(argv[1]);

    return 0;
}