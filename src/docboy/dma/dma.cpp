#include "dma.h"
#include "docboy/bus/bus.h"
#include "docboy/memory/memory.hpp"
#include "utils/bits.hpp"

Dma::Dma(Bus& bus, Oam& oam) :
    bus(bus),
    oam(oam) {
}

void Dma::transfer(uint16_t address) {
    active = true;
    cursor = 0;

    // DMA source cannot exceed 0xDF00
    if (address >= 0xE000)
        reset_bit<13>(address);
    source = address;
}

void Dma::tick() {
    if (!active)
        return;
    oam[cursor] = bus.read(source + cursor);
    active = ++cursor < Specs::MemoryLayout::OAM::SIZE;
}

void Dma::saveState(Parcel& parcel) const {
    parcel.writeBool(active);
    parcel.writeUInt16(source);
    parcel.writeUInt8(cursor);
}

void Dma::loadState(Parcel& parcel) {
    active = parcel.readBool();
    source = parcel.readUInt16();
    cursor = parcel.readUInt8();
}
