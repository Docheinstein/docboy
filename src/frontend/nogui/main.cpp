#include <chrono>
#include <iostream>

#include "docboy/cartridge/cartridge.h"
#include "docboy/cartridge/factory.h"
#include "docboy/core/core.h"
#include "docboy/cpu/cpu.h"
#include "docboy/gameboy/gameboy.h"
#include "docboy/memory/memory.h"

#include "utils/formatters.h"
#include "utils/os.h"
#include "utils/path.h"

#include "args/args.h"

#include "extra/cartridge/header.h"
#include "extra/serial/endpoints/console.h"

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/factory.h"
#endif

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"
#include "docboy/debugger/frontend.h"
#include "docboy/debugger/memwatcher.h"
#endif

namespace {
constexpr uint64_t FOREVER = UINT64_MAX;

void ensure_file_exists(const std::string& path) {
    if (!file_exists(path)) {
        std::cerr << "ERROR: failed to load '" << path << "'" << std::endl;
        exit(1);
    }
}

GameBoy gb {};
Core core {gb};
} // namespace

int main(int argc, char* argv[]) {
    struct {
        std::string rom;
        std::string boot_rom;
        uint64_t ticks_to_run {};
        bool serial {};
#ifdef ENABLE_DEBUGGER
        bool attach_debugger {};
#endif
    } args;

    Args::Parser parser {};
#ifdef ENABLE_BOOTROM
    parser.add_argument(args.boot_rom, "boot-rom").help("Boot ROM");
#endif
    parser.add_argument(args.rom, "rom").help("ROM");
    parser.add_argument(args.serial, "--serial", "-s").help("Display serial console");
    parser.add_argument(args.ticks_to_run, "-t").help("Ticks to run");
#ifdef ENABLE_DEBUGGER
    parser.add_argument(args.attach_debugger, "--debugger", "-d").help("Attach debugger");
#endif

    if (!parser.parse(argc, argv, 1)) {
        return 1;
    }

#ifdef ENABLE_BOOTROM
    ensure_file_exists(args.boot_rom);
#endif

    ensure_file_exists(args.rom);

    Path rom_path {args.rom};

#ifdef ENABLE_BOOTROM
    core.load_boot_rom(args.boot_rom);
#endif
    core.load_rom(args.rom);

    // Serial
    std::unique_ptr<SerialConsole> serial_console;
    if (args.serial) {
        serial_console = std::make_unique<SerialConsole>(std::cerr, 16);
        core.attach_serial_link(*serial_console);
    }

    const auto start = std::chrono::high_resolution_clock::now();
    for (uint64_t tick = 0; tick < args.ticks_to_run; tick += 4) {
#ifdef ENABLE_DEBUGGER
        if (core.is_asking_to_quit()) {
            break;
        }
#endif
        core.cycle();
    }
    const auto end = std::chrono::high_resolution_clock::now();
    const auto elapsed_millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (args.ticks_to_run != FOREVER) {
        uint64_t seconds_to_run = args.ticks_to_run / Specs::Frequencies::CLOCK;
        std::cout << "Time: " << elapsed_millis << std::endl;
        std::cout << "SpeedUp: " << 1000.0 * (double)seconds_to_run / (double)elapsed_millis << std::endl;
    }

    if (serial_console) {
        serial_console->flush();
    }

    return 0;
}