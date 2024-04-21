#include "args/args.h"
#include "docboy/cartridge/cartridge.h"
#include "docboy/cartridge/factory.h"
#include "docboy/core/core.h"
#include "docboy/cpu/cpu.h"
#include "docboy/gameboy/gameboy.h"
#include "docboy/memory/memory.hpp"
#include "extra/cartridge/header.h"
#include "extra/serial/endpoints/console.h"
#include "utils/formatters.hpp"
#include "utils/os.h"
#include "utils/path.h"
#include <chrono>
#include <iostream>

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/factory.h"
#endif

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"
#include "docboy/debugger/frontend.h"
#include "docboy/debugger/memsniffer.h"
#endif

static constexpr uint64_t FOREVER = UINT64_MAX;

static void dump_cartridge_info(const ICartridge& cartridge) {
    const CartridgeHeader header = CartridgeHeader::parse(cartridge);
    std::cout << "Title             :  " << header.titleAsString() << "\n";
    std::cout << "Cartridge type    :  " << hex(header.cartridge_type) << "     (" << header.cartridgeTypeDescription()
              << ")\n";
    std::cout << "Licensee (new)    :  " << hex(header.new_licensee_code) << "  ("
              << header.newLicenseeCodeDescription() << ")\n";
    std::cout << "Licensee (old)    :  " << hex(header.old_licensee_code) << "     ("
              << header.oldLicenseeCodeDescription() << ")\n";
    std::cout << "ROM Size          :  " << hex(header.rom_size) << "     (" << header.romSizeDescription() << ")\n";
    std::cout << "RAM Size          :  " << hex(header.ram_size) << "     (" << header.ramSizeDescription() << ")\n";
    std::cout << "CGB flag          :  " << hex(header.cgb_flag) << "     (" << header.cgbFlagDescription() << ")\n";
    std::cout << "SGB flag          :  " << hex(header.sgb_flag) << "\n";
    std::cout << "Destination Code  :  " << hex(header.destination_code) << "\n";
    std::cout << "Rom Version Num.  :  " << hex(header.rom_version_number) << "\n";
    std::cout << "Header checksum   :  " << hex(header.header_checksum) << "\n";
}

int main(int argc, char* argv[]) {
    struct {
        std::string rom;
        std::string bootRom;
        uint64_t ticksToRun {};
        bool serial {};
        float scaling {};
        bool dumpCartridgeInfo {};
        IF_DEBUGGER(bool debugger {});
    } args;
    check(1 > 2);

    Args::Parser parser {};
    IF_BOOTROM(parser.addArgument(args.bootRom, "boot-rom").help("Boot ROM"));
    parser.addArgument(args.rom, "rom").help("ROM");
    parser.addArgument(args.serial, "--serial", "-s").help("Display serial console");
    parser.addArgument(args.scaling, "--scaling", "-z").help("Scaling factor");
    parser.addArgument(args.dumpCartridgeInfo, "--cartridge-info", "-i").help("Dump cartridge info and quit");
    parser.addArgument(args.ticksToRun, "-t").help("Ticks to run");
    IF_DEBUGGER(parser.addArgument(args.debugger, "--debugger", "-d").help("Attach debugger"));

    if (!parser.parse(argc, argv, 1))
        return 1;

    const auto ensureExists = [](const std::string& path) {
        if (!file_exists(path)) {
            std::cerr << "ERROR: failed to load '" << path << "'" << std::endl;
            exit(1);
        }
    };

    IF_BOOTROM(ensureExists(args.bootRom));
    ensureExists(args.rom);

    std::unique_ptr<GameBoy> gb {std::make_unique<GameBoy>(IF_BOOTROM(BootRomFactory().create(args.bootRom)))};
    Core core {*gb};

    path romPath {args.rom};

    std::unique_ptr<ICartridge> cartridge {CartridgeFactory().create(romPath.string())};

    if (args.dumpCartridgeInfo) {
        dump_cartridge_info(*cartridge);
        return 0;
    }

    core.loadRom(std::move(cartridge));

#ifdef ENABLE_SERIAL
    std::unique_ptr<SerialConsole> serialConsole;
    std::unique_ptr<SerialLink> serialLink;
    if (args.serial) {
        serialConsole = std::make_unique<SerialConsole>(std::cerr, 16);
        serialLink = std::make_unique<SerialLink>();
        serialLink->plug1.attach(*serialConsole);
        core.attachSerialLink(serialLink->plug2);
    }
#endif

    const auto tStart = std::chrono::high_resolution_clock::now();
    for (uint64_t tick = 0; tick < args.ticksToRun; tick += 4) {
#ifdef ENABLE_DEBUGGER
        if (core.isDebuggerAskingToShutdown())
            break;
#endif
        core.cycle();
    }
    const auto tEnd = std::chrono::high_resolution_clock::now();
    const auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();

    if (args.ticksToRun != FOREVER) {
        uint64_t secondsToRun = args.ticksToRun / Specs::Frequencies::CLOCK;
        std::cout << "Time: " << elapsedMillis << std::endl;
        std::cout << "SpeedUp: " << 1000.0 * (double)secondsToRun / (double)elapsedMillis << std::endl;
    }

#ifdef ENABLE_SERIAL
    if (serialConsole) {
        serialConsole->flush();
    }
#endif

    return 0;
}