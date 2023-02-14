#include "bus.h"
#include "log/log.h"
#include "utils/binutils.h"
#include "memory.h"
#include "cartridge.h"

IBus::~IBus() {

}

Bus::Bus(Cartridge &cartridge, IMemory &wram1, IMemory &wram2) :
    cartridge(cartridge),
    wram1(wram1),
    wram2(wram2) {

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
        throw std::runtime_error("Read at address " + hex(addr) + " not implemented");
    } else if (addr < 0xD000) {
        result = wram1[addr - 0xC000];
    } else if (addr < 0xE000) {
        result = wram2[addr - 0xD000];
    } else if (addr < 0xFE00) {
        throw std::runtime_error("Read at address " + hex(addr) + " is not allowed");
    } else if (addr < 0xFEA0) {
        DEBUG(1) << "WARN: (OAM) read at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xFF00) {
        throw std::runtime_error("Read at address " + hex(addr) + " is not allowed");
    } else if (addr < 0xFF80) {
        DEBUG(1) << "WARN: (IO) read at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xFFFF) {
        DEBUG(1) << "WARN: (HRAM) read at address " + hex(addr) + " not implemented" << std::endl;
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
        throw std::runtime_error("Write at address " + hex(addr) + " not implemented");
    } else if (addr < 0xD000) {
        wram1[addr - 0xC000] = value;
    } else if (addr < 0xE000) {
        wram2[addr - 0xD000] = value;
    } else if (addr < 0xFE00) {
        throw std::runtime_error("Write at address " + hex(addr) + " is not allowed");
    } else if (addr < 0xFEA0) {
        DEBUG(1) << "WARN: (OAM) write at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xFF00) {
        throw std::runtime_error("Write at address " + hex(addr) + " is not allowed");
    } else if (addr < 0xFF80) {
        DEBUG(1) << "WARN: (IO) write at address " + hex(addr) + " not implemented" << std::endl;
    } else if (addr < 0xFFFF) {
        DEBUG(1) << "WARN: (HRAM) write at address " + hex(addr) + " not implemented" << std::endl;
    } else /* addr == 0xFFF */ {
        DEBUG(1) << "WARN: (IE) write at address " + hex(addr) + " not implemented" << std::endl;
    }
}