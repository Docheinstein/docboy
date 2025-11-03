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
                            .check_interval_ticks(DEFAULT_CHECK_INTERVAL)
                            .max_ticks(DEFAULT_MAX_DURATION)
                            .force_check(true)
                            .expect_framebuffer(argv[2])
                            .run();

    return result ? 0 : 1;
}