#include "dma.h"
#include "docboy/memory/memory.hpp"
#include "docboy/mmu/mmu.h"
#include "utils/bits.hpp"

/* DMA request timing example.
 *
 *  t        CPU                   DMA
 * CPU | write FF46        None -> Requested
 * DMA |                   Requested -> Pending
 *
 * CPU | [OAM reads FF]
 * DMA |                   Pending -> Transferring
 *
 * CPU | [OAM reads FF]
 * DMA |                   Transfer first byte
 */

Dma::Dma(Mmu& mmu, Oam& oam) :
    mmu(mmu),
    oam(oam) {
}

void Dma::startTransfer(uint16_t address) {
    request.state = RequestState::Requested; // delay DMA start by one m-cycle

    // DMA source cannot exceed 0xDF00
    if (address >= 0xE000)
        reset_bit<13>(address);
    request.source = address;
}

void Dma::tick() {
    if (transferring) {
        // TODO: use requestRead() with exact DMA timing instead of read()
        oam[cursor] = mmu.read(source + cursor);
        transferring = ++cursor < Specs::MemoryLayout::OAM::SIZE;
    }

    if (request.state != RequestState::None) {
        request.state = (RequestState)((uint8_t)request.state - 1);
        if (request.state == RequestState::None) {
            transferring = true;
            source = request.source;
            cursor = 0;
        }
    }
}

void Dma::saveState(Parcel& parcel) const {
    parcel.writeUInt8((uint8_t)request.state);
    parcel.writeUInt16(request.source);
    parcel.writeBool(transferring);
    parcel.writeUInt16(source);
    parcel.writeUInt8(cursor);
}

void Dma::loadState(Parcel& parcel) {
    request.state = (RequestState)parcel.readUInt8();
    request.source = parcel.readUInt16();
    transferring = parcel.readBool();
    source = parcel.readUInt16();
    cursor = parcel.readUInt8();
}