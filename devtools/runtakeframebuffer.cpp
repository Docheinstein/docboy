#include <iostream>

#include "testutils/runners.h"

// usage: runtakeframebuffer <rom> <ticks> <framebuffer>

std::string boot_rom;

int main(int argc, const char* argv[]) {
    if (argc < 4) {
        std::cerr << "usage: runtakeframebuffer <rom> <ticks> <framebuffer>" << std::endl;
        exit(1);
    }

#ifdef ENABLE_BOOTROM
    const char* var = std::getenv("DOCBOY_BIOS");
    if (!var) {
        std::cerr << "Please set the boot ROM with with 'DOCBOY_BIOS' environment variable." << std::endl;
        return 1;
    }
    boot_rom = var;
#endif

    // Run for the given amount of ticks
    SimpleRunner runner;
    runner.rom(argv[1]).max_ticks(*strtou(argv[2])).run();

    // Save the framebuffer
    const auto framebuffer_path = Path {argv[3]}.string();
    save_framebuffer_png(framebuffer_path, runner.gb->lcd.get_pixels());

    return 0;
}