#include "bus.h"
#include "log/log.h"
#include "utils/binutils.h"
#include "memory.h"
#include "cartridge.h"

IBus::~IBus() {

}

Bus::Bus(Cartridge &cartridge, IMemory &wram1, IMemory &wram2, IMemory &io, IMemory &hram) :
    cartridge(cartridge), wram1(wram1), wram2(wram2), io(io), hram(hram) {

}

Bus::~Bus() {

}


uint8_t Bus::read(uint16_t addr) {
    uint8_t result = 0;
    if (addr < 0x4000) {
        result = cartridge[addr];
    } else if (addr < 0x8000) {
        // TODO: MBC
        result = cartridge[addr];
    } else if (addr < 0xA000) {
        DEBUG(1) << "WARN: (VRAM) read at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xC000) {
        DEBUG(1) << "WARN: (External RAM) read at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xD000) {
        result = wram1.read(addr - 0xC000);
    } else if (addr < 0xE000) {
        result = wram2.read(addr - 0xD000);
    } else if (addr < 0xFE00) {
        throw std::runtime_error("Read at address " + hex(addr) + " is not allowed");
    } else if (addr < 0xFEA0) {
        DEBUG(1) << "WARN: (OAM) read at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xFF00) {
        throw std::runtime_error("Read at address " + hex(addr) + " is not allowed");
    } else if (addr < 0xFF80) {
        result = io.read(addr - 0xFF00);
    } else if (addr < 0xFFFF) {
        result = hram.read(addr - 0xFF80);
    } else /* addr == 0xFFF */ {
        DEBUG(1) << "WARN: (IE) read at address " + hex(addr) + " not implemented" << std::endl;
    }
    DEBUG(2) << "Bus:read(" << hex(addr) << ") -> " << hex(result) << std::endl;
    return result;
}

void Bus::write(uint16_t addr, uint8_t value) {
    DEBUG(2) << "Bus:write(" << hex(addr) << ", " << hex(value) << ")" << std::endl;
    if (addr < 0x4000) {
        throw std::runtime_error("Invalid write. Memory address " + hex(addr) + " is read only");
    } else if (addr < 0x8000) {
        throw std::runtime_error("Invalid write. Memory address " + hex(addr) + " is read only");
    } else if (addr < 0xA000) {
        DEBUG(1) << "WARN: (VRAM) write at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xC000) {
        DEBUG(1) << "WARN: (External RAM) write at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xD000) {
        wram1.write(addr - 0xC000, value);
    } else if (addr < 0xE000) {
        wram2.write(addr - 0xD000, value);
    } else if (addr < 0xFE00) {
        throw std::runtime_error("Write at address " + hex(addr) + " is not allowed");
    } else if (addr < 0xFEA0) {
        DEBUG(1) << "WARN: (OAM) write at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xFF00) {
        throw std::runtime_error("Write at address " + hex(addr) + " is not allowed");
    } else if (addr < 0xFF80) {
        io.write(addr - 0xFF00, value);
    } else if (addr < 0xFFFF) {
        hram.write(addr - 0xFF80, value);
    } else /* addr == 0xFFF */ {
        DEBUG(1) << "WARN: (IE) write at address " + hex(addr) + " not implemented" << std::endl;
    }
}

ObservableBus::Observer::~Observer() {

}

ObservableBus::ObservableBus(Cartridge &cartridge, IMemory &wram1, IMemory &wram2, IMemory &io, IMemory &hram)
    : Bus(cartridge, wram1, wram2, io, hram) {

}

ObservableBus::~ObservableBus() {

}

void ObservableBus::setObserver(ObservableBus::Observer *obs) {
    observer = obs;
}

void ObservableBus::unsetObserver() {
    observer = nullptr;
}

uint8_t ObservableBus::read(uint16_t addr) {
    uint8_t value = Bus::read(addr);
    if (observer)
        observer->onBusRead(addr, value);
    return value;
}

void ObservableBus::write(uint16_t addr, uint8_t value) {
    uint8_t oldValue = Bus::read(addr);
    Bus::write(addr, value);
    if (observer)
        observer->onBusWrite(addr, oldValue, value);
}
