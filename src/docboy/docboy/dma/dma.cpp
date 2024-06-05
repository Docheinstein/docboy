#include "docboy/dma/dma.h"

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

Dma::Dma(Mmu::View<Device::Dma> mmu, OamBus::View<Device::Dma> oam_bus) :
    mmu {mmu},
    oam {oam_bus} {
}

void Dma::start_transfer(uint16_t address) {
    request.state = RequestState::Requested;

    // DMA source cannot exceed 0xDF00
    if (address >= 0xE000) {
        reset_bit<13>(address);
    }

    request.source = address;
}

void Dma::tick_t1() {
    if (transferring) {
        // Complete read request and actually read source data
        const uint8_t src_data = mmu.flush_read_request();

        // Actually write to OAM
        oam.flush_write_request(src_data);

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
            mmu.read_request(source + cursor);

            // Start a write request to the OAM bus.
            // Note that the request is initiated here: this means that if the PPU
            // will access OAM during the next t0 and t1, conflicts will happen
            // (likely PPU will read from the address written by DMA now).
            // [hacktix/strikethrough]
            oam.write_request(Specs::MemoryLayout::OAM::START + cursor);
        } else {
            // DMA transfer completed: release OAM bus
            oam.release();
            transferring = false;
        }
    }
}

void Dma::save_state(Parcel& parcel) const {
    parcel.write_uint8(request.state);
    parcel.write_uint16(request.source);
    parcel.write_bool(transferring);
    parcel.write_uint16(source);
    parcel.write_uint8(cursor);
}

void Dma::load_state(Parcel& parcel) {
    request.state = parcel.read_uint8();
    request.source = parcel.read_uint16();
    transferring = parcel.read_bool();
    source = parcel.read_uint16();
    cursor = parcel.read_uint8();
}
void Dma::reset() {
    request.state = RequestState::None;
    request.source = 0;
    transferring = false;
    source = 0;
    cursor = 0;
}
