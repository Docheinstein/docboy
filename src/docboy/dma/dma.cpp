#include "dma.h"
#include "docboy/bus/bus.h"
#include "docboy/memory/memory.hpp"

Dma::Dma(Bus& bus, Oam& oam) :
    bus(bus),
    oam(oam) {
}

void Dma::transfer(uint16_t address) {
    // TODO: what does happen if a transfer is already in progress?
    active = true;
    source = address;
    cursor = 0;
}

void Dma::tick() {
    if (!active)
        return;

    // DMA source cannot exceed 0xDF00 (copied from mGBA)
    uint16_t src = source + cursor;
    if (src >= 0xE000)
        src &= ~0x2000;

    oam[cursor] = bus.read(src);
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
