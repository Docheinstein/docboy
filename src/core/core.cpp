#include "core.h"
#include "utils/log.h"
#include "utils/binutils.h"
#include "definitions.h"
#include <vector>
#include <algorithm>
#include "cartridge/cartridgefactory.h"
#include "core/ppu/ppu.h"


Core::Core(GameBoy &gameboy):
    gameboy(gameboy),
    divCounter(),
    timaCounter(),
    running(true), // TODO: bad here
    clk() {
}

bool Core::loadROM(const std::string &rom) {
    auto c = CartridgeFactory::makeCartridge(rom);
    if (!c)
        return false;
    attachCartridge(std::move(c));

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

void Core::attachCartridge(std::unique_ptr<Cartridge> cartridge) {
    gameboy.cartridge = std::move(cartridge);
    gameboy.bus->attachCartridge(gameboy.cartridge.get());
}

void Core::start() {
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

bool Core::tick() {
    auto now = std::chrono::high_resolution_clock::now();
    auto nextTick = lastTick + std::chrono::nanoseconds(1000000000 / Specs::FREQUENCY);
    while (now < nextTick) {
        now = std::chrono::high_resolution_clock::now();
    }

    IPPU &ppu = *gameboy.ppu;
    ppu.tick();

    if (clk == 0) {
        ICPU &cpu = *gameboy.cpu;
        IBus &bus = *gameboy.bus;

        cpu.tick();

        // DIV
        divCounter += 1;
        if (divCounter >= Specs::CPU::FREQUENCY / Specs::CPU::DIV_FREQUENCY) {
            divCounter = 0;
            uint8_t DIV = bus.read(Registers::Timers::DIV);
            bus.write(Registers::Timers::DIV, DIV + 1);
        }

        // TIMA
        uint8_t TAC = bus.read(Registers::Timers::TAC);
        if (get_bit<Bits::Registers::Timers::TAC::ENABLE>(TAC))
            timaCounter += 1;

        if (timaCounter >= Specs::CPU::FREQUENCY / Specs::CPU::TAC_FREQUENCY[bitmasked<2>(TAC)]) {
            timaCounter = 0;
            uint8_t TIMA = bus.read(Registers::Timers::TIMA);
            auto [result, overflow] = sum_carry<7>(TIMA, 1);
            if (overflow) {
                TIMA = bus.read(Registers::Timers::TMA);
                bus.write(Registers::Timers::TIMA, TIMA);

                uint8_t IF = bus.read(Registers::Interrupts::IF);
                set_bit<Bits::Interrupts::TIMER>(IF, true);
                bus.write(Registers::Interrupts::IF, IF);
            } else {
                bus.write(Registers::Timers::TIMA, result);
            }
        }

        // Serial
        if (serialLink) {
            uint8_t SC = bus.read(Registers::Serial::SC);
            if (get_bit<7>(SC) && get_bit<0>(SC))
                serialLink->tick();
        }
    }

    clk = (clk + 1) % 4;
    lastTick = now;

    return running;
}

uint8_t Core::serialRead() {
    return gameboy.bus->read(Registers::Serial::SB);
}

void Core::serialWrite(uint8_t data) {
    IBus &bus = *gameboy.bus;
    bus.write(Registers::Serial::SB, data);
    uint8_t SC = bus.read(Registers::Serial::SC);
    set_bit<7>(SC, false);
    bus.write(Registers::Serial::SC, SC);
    // TODO: interrupt
}
