#include "gible.h"
#include "log/log.h"
#include "utils/binutils.h"

#include <fstream>
#include <vector>
#include <cstring>

Gible::Gible() : bus(cartridge, memory), cpu(bus) {

}

bool Gible::loadROM(const std::string &rom) {
    DEBUG(1) << "Loading ROM: " << rom << std::endl;

    auto c = Cartridge::fromFile(rom);
    if (!c)
        return false;
    cartridge = std::move(*c);

#if DEBUG_LEVEL >= 1
    auto h = cartridge.header();
    DEBUG(1)
        << "ROM loaded\n"
        << "---------------\n"
        << cartridge << "\n"
        << "---------------\n"
        << "Entry point: " << hex(h.entry_point) << "\n"
        << "Title: " << h.title << "\n"
        << "CGB flag: " << hex(h.cgb_flag) << "\n"
        << "New licensee code: " << hex(h.new_licensee_code) << "\n"
        << "SGB flag: " << hex(h.sgb_flag) << "\n"
        << "Cartridge type: " << hex(h.cartridge_type) << "\n"
        << "ROM size: " << hex(h.rom_size) << "\n"
        << "RAM size: " << hex(h.ram_size) << "\n"
        << "Destination code: " << hex(h.destination_code) << "\n"
        << "Old licensee code: " << hex(h.old_licensee_code) << "\n"
        << "ROM version number: " << hex(h.rom_version_number) << "\n"
        << "Header checksum: " << hex(h.header_checksum) << "\n"
        << "Global checksum: " << hex(h.global_checksum) << "\n"
        << "---------------" << std::endl;

#endif
    return true;
}

void Gible::start() {
    DEBUG(1) << "Starting " << std::endl;
    while (std::cin) {
        cpu.tick();
        std::cout << "Press ENTER to continue..." << std::endl;
        std::cin.get();
        std::cout << std::endl;
        // sleep
    }
}
