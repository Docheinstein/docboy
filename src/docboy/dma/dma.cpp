#include "dma.h"
#include "docboy/memory/memory.hpp"

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

Dma::Dma(MmuView mmu, OamBusView oamBus) :
    mmu(mmu),
    oam(oamBus) {
}

void Dma::startTransfer(uint16_t address) {
    request.state = RequestState::Requested;

    // DMA source cannot exceed 0xDF00
    if (address >= 0xE000)
        reset_bit<13>(address);
    request.source = address;
}

void Dma::tickRead() {
    if (request.state != RequestState::None) {
        if (--request.state == RequestState::None) {
            oam.acquire();
            transferring = true;
            source = request.source;
            cursor = 0;
        }
    }

    if (transferring) {
        if (cursor < Specs::MemoryLayout::OAM::SIZE) {
            mmu.requestRead(source + cursor, data);
        } else {
            transferring = false;
            oam.release();
        }
    }
}

void Dma::tickWrite() {
    if (transferring) {
        oam[cursor] = data;
        cursor++;
    }
}

void Dma::saveState(Parcel& parcel) const {
    parcel.writeUInt8(request.state);
    parcel.writeUInt16(request.source);
    parcel.writeBool(transferring);
    parcel.writeUInt16(source);
    parcel.writeUInt8(cursor);
    parcel.writeUInt8(data);
}

void Dma::loadState(Parcel& parcel) {
    request.state = parcel.readUInt8();
    request.source = parcel.readUInt16();
    transferring = parcel.readBool();
    source = parcel.readUInt16();
    cursor = parcel.readUInt8();
    data = parcel.readUInt8();
}
