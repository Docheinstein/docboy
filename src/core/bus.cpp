#include "bus.h"
#include "log/log.h"
#include "utils/binutils.h"
#include "memory.h"
#include "cartridge.h"

IBus::~IBus() {

}

BusImpl::BusImpl(Cartridge &cartridge, Memory &memory) : cartridge(cartridge), memory(memory) {

}

BusImpl::~BusImpl() {

}


uint8_t BusImpl::read(uint16_t addr) {
    DEBUG(1) << "Bus:read(" << hex(addr) << ")" << std::endl;
    if (addr < 0x4000) {
        return cartridge[addr];
    } else if (addr < 0x8000) {
        // TODO: MBC
        return cartridge[addr];
    } else {
        throw std::runtime_error("Read at address " + hex(addr) + " not implemented");
    }
}

void BusImpl::write(uint16_t addr, uint8_t value) {
    DEBUG(1) << "Bus:write(" << hex(value) << ")" << std::endl;
    if (addr < 0x4000) {
        throw std::runtime_error("Invalid write. Memory address " + hex(addr) + " is read only");
    } else if (addr < 0x8000) {
        throw std::runtime_error("Invalid write. Memory address " + hex(addr) + " is read only");
    } else {
        throw std::runtime_error("Write at address " + hex(addr) + " not implemented");
    }
}