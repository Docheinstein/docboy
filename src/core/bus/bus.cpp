#include "bus.h"
#include "utils/log.h"
#include "utils/binutils.h"
#include "core/definitions.h"
#include "core/impl/memory.h"

Bus::Bus(IMemory &vram, IMemory &wram1, IMemory &wram2, IMemory &oam, IMemory &io, IMemory &hram, IMemory &ie) :
     cartridge(), vram(vram), wram1(wram1), wram2(wram2), oam(oam), io(io), hram(hram), ie(ie) {

}

uint8_t Bus::read(uint16_t addr) const {
    if (addr <= MemoryMap::ROM1::END) {
        return cartridge->read(addr);
    } else if (addr <= MemoryMap::VRAM::END) {
        return vram.read(addr - MemoryMap::VRAM::START);
    } else if (addr <= MemoryMap::RAM::END) {
        return cartridge->read(addr);
    } else if (addr <= MemoryMap::WRAM1::END) {
        return wram1.read(addr - MemoryMap::WRAM1::START);
    } else if (addr <= MemoryMap::WRAM2::END) {
        return wram2.read(addr - MemoryMap::WRAM2::START);
    } else if (addr <= MemoryMap::ECHO_RAM::END) {
        WARN() << "Read at address " + hex(addr) + " is not allowed (ECHO RAM)" << std::endl;
    } else if (addr <= MemoryMap::OAM::END) {
        return oam.read(addr - MemoryMap::OAM::START);
    } else if (addr <= MemoryMap::NOT_USABLE::END) {
        WARN() << "Read at address " + hex(addr) + " is not allowed (NOT USABLE)" << std::endl;
    } else if (addr <= MemoryMap::IO::END) {
        return io.read(addr - MemoryMap::IO::START);
    } else if (addr <= MemoryMap::HRAM::END) {
        return hram.read(addr - MemoryMap::HRAM::START);
    } else if (addr == MemoryMap::IE) {
        return ie.read(0);
    }
    return 0xFF;
}

void Bus::write(uint16_t addr, uint8_t value) {
    if (addr <= MemoryMap::ROM1::END) {
        cartridge->write(addr, value);
    } else if (addr <= MemoryMap::VRAM::END) {
        vram.write(addr - MemoryMap::VRAM::START, value);
    } else if (addr <= MemoryMap::RAM::END) {
        cartridge->write(addr, value);
    } else if (addr <= MemoryMap::WRAM1::END) {
        wram1.write(addr - MemoryMap::WRAM1::START, value);
    } else if (addr <= MemoryMap::WRAM2::END) {
        wram2.write(addr - MemoryMap::WRAM2::START, value);
    } else if (addr <= MemoryMap::ECHO_RAM::END) {
        WARN() << "Write at address " + hex(addr) + " is not allowed (ECHO RAM)" << std::endl;
    } else if (addr <= MemoryMap::OAM::END) {
        oam.write(addr - MemoryMap::OAM::START, value);
    } else if (addr <= MemoryMap::NOT_USABLE::END) {
        WARN() << "Write at address " + hex(addr) + " is not allowed (NOT USABLE)" << std::endl;
    } else if (addr <= MemoryMap::IO::END) {
        io.write(addr - MemoryMap::IO::START, value);
    } else if (addr <= MemoryMap::HRAM::END) {
        hram.write(addr - MemoryMap::HRAM::START, value);
    } else if (addr == MemoryMap::IE) {
        ie.write(0, value);
    }
}

void Bus::attachCartridge(IMemory *c) {
    cartridge = c;
}

void Bus::detachCartridge() {
    cartridge = nullptr;
}

