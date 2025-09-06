#include <iostream>

#include "testutils/runners.h"

// usage: runcmpframebuffer <rom> <framebuffer>

int main(int argc, const char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: runcmpframebuffer <rom> <framebuffer>" << std::endl;
        exit(1);
    }

    const bool result = FramebufferRunner()
                            .rom(argv[1])
                            .check_interval_ticks(DURATION_VERY_SHORT)
                            .max_ticks(DURATION_SHORT)
                            .force_check(true)
                            .expect_framebuffer(argv[2])
                            .run();

    return result ? 0 : 1;
}