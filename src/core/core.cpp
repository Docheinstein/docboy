#include "core.h"
#include "utils/log.h"
#include "utils/binutils.h"
#include "definitions.h"
#include <vector>
#include <algorithm>
#include "cartridge/cartridgefactory.h"


Core::Core() :
    divCounter(),
    timaCounter() {
}


bool Core::loadROM(const std::string &rom) {
    DEBUG(1) << "Loading ROM: " << rom << std::endl;

    auto c = CartridgeFactory::makeCartridge(rom);
    if (!c)
        return false;
    gameboy.cartridge = std::move(c);
    gameboy.bus.attachCartridge(gameboy.cartridge.get());

//#if DEBUG_LEVEL >= 1
//    auto h = cartridge->header();
//    DEBUG(1)
//        << "ROM loaded\n"
//        << "---------------\n"
//        << cartridge << "\n"
//        << "---------------\n"
//        << "Entry point: " << hex(h.entry_point) << "\n"
//        << "Title: " << h.title << "\n"
//        << "CGB flag: " << hex(h.cgb_flag) << "\n"
//        << "New licensee code: " << hex(h.new_licensee_code) << "\n"
//        << "SGB flag: " << hex(h.sgb_flag) << "\n"
//        << "Cartridge type: " << hex(h.cartridge_type) << "\n"
//        << "ROM size: " << hex(h.rom_size) << "\n"
//        << "RAM size: " << hex(h.ram_size) << "\n"
//        << "Destination code: " << hex(h.destination_code) << "\n"
//        << "Old licensee code: " << hex(h.old_licensee_code) << "\n"
//        << "ROM version number: " << hex(h.rom_version_number) << "\n"
//        << "Header checksum: " << hex(h.header_checksum) << "\n"
//        << "Global checksum: " << hex(h.global_checksum) << "\n"
//        << "---------------" << std::endl;
//
//#endif
    return true;
}

void Core::start() {
    running = true;
    while (running) {
        tick();
    }
}

void Core::stop() {
    running = false;
}

void Core::attachSerialLink(std::shared_ptr<SerialLink> serial) {
    serialLink = serial;
}

void Core::detachSerialLink() {
    serialLink = nullptr;
}

void Core::tick() {
    Cpu &cpu = gameboy.cpu;
    Bus &bus = gameboy.bus;

    cpu.tick();

    // DIV
    divCounter += 1;
    if (divCounter >= Frequencies::Cpu / Frequencies::DIV) {
        divCounter = 0;
        uint8_t DIV = bus.read(MemoryMap::IO::DIV);
        bus.write(MemoryMap::IO::DIV, DIV + 1);
    }

    // TIMA
    uint8_t TAC = bus.read(MemoryMap::IO::TAC);
    if (get_bit<Bits::IO::TAC::ENABLE>(TAC))
        timaCounter += 1;

    if (timaCounter >= Frequencies::Cpu / Frequencies::TAC_SELECTOR[bitmasked<2>(TAC)]) {
        timaCounter = 0;
        uint8_t TIMA = bus.read(MemoryMap::IO::TIMA);
        auto [result, overflow] = sum_carry<7>(TIMA, 1);
        if (overflow) {
            TIMA = bus.read(MemoryMap::IO::TMA);
            bus.write(MemoryMap::IO::TIMA, TIMA);

            uint8_t IF = bus.read(MemoryMap::IO::IF);
            set_bit<Bits::Interrupts::TIMER>(IF, true);
            bus.write(MemoryMap::IO::IF, IF);
        } else {
            bus.write(MemoryMap::IO::TIMA, result);
        }
    }

    // Serial
    if (serialLink) {
        uint8_t SC = bus.read(MemoryMap::IO::SC);
        if (get_bit<7>(SC) && get_bit<0>(SC))
            serialLink->tick();
    }
}

uint8_t Core::serialRead() {
    return gameboy.bus.read(MemoryMap::IO::SB);
}

void Core::serialWrite(uint8_t data) {
    Bus &bus = gameboy.bus;
    bus.write(MemoryMap::IO::SB, data);
    uint8_t SC = bus.read(MemoryMap::IO::SC);
    set_bit<7>(SC, false);
    bus.write(MemoryMap::IO::SC, SC);
    // TODO: interrupt
}