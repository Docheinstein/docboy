#include "bus.h"
#include "utils/log.h"
#include "utils/binutils.h"
#include "memory.h"


Bus::Bus(IMemory &wram1, IMemory &wram2, IMemory &io, IMemory &hram, IMemory &ie) :
    wram1(wram1), wram2(wram2), io(io), hram(hram), ie(ie), cartridge() {

}


uint8_t Bus::read(uint16_t addr) const {
    if (addr < 0x8000) {
        return cartridge->read(addr);
    } else if (addr < 0xA000) {
        DEBUG(1) << "WARN: (VRAM) read at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xC000) {
        return cartridge->read(addr);
    } else if (addr < 0xD000) {
        return wram1.read(addr - 0xC000);
    } else if (addr < 0xE000) {
        return wram2.read(addr - 0xD000);
    } else if (addr < 0xFE00) {
        throw std::runtime_error("Read at address " + hex(addr) + " is not allowed");
    } else if (addr < 0xFEA0) {
        DEBUG(1) << "WARN: (OAM) read at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xFF00) {
        throw std::runtime_error("Read at address " + hex(addr) + " is not allowed");
    } else if (addr < 0xFF80) {
        return io.read(addr - 0xFF00);
    } else if (addr < 0xFFFF) {
        return hram.read(addr - 0xFF80);
    } else if (addr == 0xFFFF) {
        return ie.read(0);
    }
    return 0xFF;
}

void Bus::write(uint16_t addr, uint8_t value) {
    if (addr < 0x8000) {
        cartridge->write(addr, value);
    } else if (addr < 0xA000) {
        DEBUG(1) << "WARN: (VRAM) write at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xC000) {
        cartridge->write(addr, value);
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
    } else if (addr == 0xFFFF) {
        ie.write(0, value);
    }
}

void Bus::attachCartridge(IMemory *c) {
    cartridge = c;
}

void Bus::detachCartridge() {
    cartridge = nullptr;
}

