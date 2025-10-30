#include <chrono>
#include <iostream>

#include "docboy/cartridge/cartridge.h"
#include "docboy/cartridge/factory.h"
#include "docboy/core/core.h"

#include "utils/os.h"
#include "utils/path.h"

#include "args/args.h"

namespace {
GameBoy gb {};
Core core {gb};
} // namespace

int main(int argc, char* argv[]) {
    struct {
        std::string rom;
        uint64_t frames_to_run {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.rom, "rom").help("ROM");
    parser.add_argument(args.frames_to_run, "frames").help("FRAMES");

    if (!parser.parse(argc, argv, 1)) {
        return 1;
    }

    if (!file_exists(args.rom)) {
        std::cerr << "ERROR: failed to load '" << args.rom << "'" << std::endl;
        exit(1);
    }

    core.load_rom(args.rom);

    core.set_audio_sample_rate(32768);

    const auto start = std::chrono::high_resolution_clock::now();
    for (uint64_t f = 0; f < args.frames_to_run; f++) {
        core.frame();
    }
    const auto end = std::chrono::high_resolution_clock::now();
    const auto elapsed_millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    const auto speedup = [](uint64_t frames, auto millis) {
        return 1000.0 * (double)frames / (Specs::FPS * millis);
    };

    std::cout << "Time     : " << elapsed_millis << "ms" << std::endl;
    std::cout << "Frames   : " << args.frames_to_run << std::endl;
    std::cout << "Speed Up : " << speedup(args.frames_to_run, elapsed_millis) << std::endl;
    std::cout << "E[FPS]   : " << Specs::FPS * speedup(args.frames_to_run, elapsed_millis) << std::endl;

    return 0;
}