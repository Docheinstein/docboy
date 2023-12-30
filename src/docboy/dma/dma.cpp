#include "dma.h"

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

Dma::Dma(Mmu::View<Device::Dma> mmu, OamBus::View<Device::Dma> oamBus) :
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

void Dma::tick_t1() {
    if (transferring) {
        // Complete read request and actually read source data
        const uint8_t srcData = mmu.flushReadRequest();

        // Actually write to OAM
        oam.flushWriteRequest(srcData);

        cursor++;
    }
}

void Dma::tick_t3() {
    if (request.state != RequestState::None) {
        if (--request.state == RequestState::None) {
            // Begin DMA transfer
            oam.acquire();
            transferring = true;
            source = request.source;
            cursor = 0;
        }
    }

    if (transferring) {
        if (cursor < Specs::MemoryLayout::OAM::SIZE) {
            // Start a read request for the source data
            mmu.readRequest(source + cursor);

            // Start a write request to the OAM bus.
            // Note that the request is initiated here: this means that if the PPU
            // will access OAM during the next t0 and t1, conflicts will happen
            // (likely PPU will read from the address written by DMA now).
            // [hacktix/strikethrough]
            oam.writeRequest(Specs::MemoryLayout::OAM::START + cursor);
        } else {
            // DMA transfer completed: release OAM bus
            oam.release();
            transferring = false;
        }
    }
}

void Dma::saveState(Parcel& parcel) const {
    parcel.writeUInt8(request.state);
    parcel.writeUInt16(request.source);
    parcel.writeBool(transferring);
    parcel.writeUInt16(source);
    parcel.writeUInt8(cursor);
}

void Dma::loadState(Parcel& parcel) {
    request.state = parcel.readUInt8();
    request.source = parcel.readUInt16();
    transferring = parcel.readBool();
    source = parcel.readUInt16();
    cursor = parcel.readUInt8();
}
