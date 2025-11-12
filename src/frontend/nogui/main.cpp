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

void dump_cartridge_info(const CartridgeHeader& header) {
    using namespace CartridgeHeaderHelpers;

    std::cout << "Title             :  " << title_as_string(header) << "\n";
    std::cout << "Cartridge Type    :  " << hex(header.cartridge_type) << "     (" << cartridge_type_description(header)
              << ")\n";
    std::cout << "Licensee (old)    :  " << hex(header.old_licensee_code) << "     ("
              << old_licensee_code_description(header) << ")\n";
    std::cout << "Licensee (new)    :  " << new_licensee_code_as_string(header) << "     ("
              << new_licensee_code_description(header) << ")\n";
    std::cout << "ROM Size          :  " << hex(header.rom_size) << "     (" << rom_size_description(header) << ")\n";
    std::cout << "RAM Size          :  " << hex(header.ram_size) << "     (" << ram_size_description(header) << ")\n";
    std::cout << "CGB Flag          :  " << hex(cgb_flag(header)) << "     (" << cgb_flag_description(header) << ")\n";
    std::cout << "SGB Flag          :  " << hex(header.sgb_flag) << "\n";
    std::cout << "Destination Code  :  " << hex(header.destination_code) << "\n";
    std::cout << "Rom Version Num.  :  " << hex(header.rom_version_number) << "\n";
    std::cout << "Header Checksum   :  " << hex(header.header_checksum) << "\n";
    std::cout << "Title Checksum    :  " << hex(title_checksum(header)) << "\n";
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
        float scaling {};
        bool dump_cartridge_info {};
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
    parser.add_argument(args.scaling, "--scaling", "-z").help("Scaling factor");
    parser.add_argument(args.dump_cartridge_info, "--cartridge-info", "-i").help("Dump cartridge info and quit");
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

    if (args.dump_cartridge_info) {
        dump_cartridge_info(CartridgeFactory::create_header(rom_path.string()));
        return 0;
    }

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