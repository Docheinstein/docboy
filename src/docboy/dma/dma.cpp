#include "dma.h"
#include "docboy/memory/memory.hpp"
#include "docboy/mmu/mmu.h"
#include "utils/asserts.h"
#include "utils/bits.hpp"

Dma::Dma(Mmu& mmu, Oam& oam) :
    mmu(mmu),
    oam(oam) {
}

void Dma::transfer(uint16_t address) {
    state = TransferState::Pending; // delay DMA start by one m-cycle
    cursor = 0;

    // DMA source cannot exceed 0xDF00
    if (address >= 0xE000)
        reset_bit<13>(address);
    source = address;
}

void Dma::tick() {
    if (state == TransferState::None) {
        return;
    } else if (state == TransferState::Active) {
        oam[cursor] = mmu.read(source + cursor);
        if (++cursor >= Specs::MemoryLayout::OAM::SIZE)
            state = TransferState::Finished;
    } else if (state == TransferState::Pending) {
        state = TransferState::Active;
    } else if (state == TransferState::Finished) {
        state = TransferState::None;
    }
}

void Dma::saveState(Parcel& parcel) const {
    parcel.writeUInt8(static_cast<uint8_t>(state));
    parcel.writeUInt16(source);
    parcel.writeUInt8(cursor);
}

void Dma::loadState(Parcel& parcel) {
    state = static_cast<TransferState>(parcel.readUInt8());
    source = parcel.readUInt16();
    cursor = parcel.readUInt8();
}