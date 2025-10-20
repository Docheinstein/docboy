#include "docboy/dma/dma.h"

Dma::Dma(Mmu::View<Device::Dma> mmu, OamBus::View<Device::Dma> oam_bus) :
    mmu {mmu},
    oam {oam_bus} {
    reset();
}

void Dma::start_transfer(uint8_t value) {
    uint16_t address = value << 8;

    // DMA source cannot exceed 0xDF00
    if (address >= 0xE000) {
        reset_bit<13>(address);
    }

    // It seems that DMA takes 4 DMA ticks before effectively start.
    // That is, 8 T-cycles in Normal Speed, and 4 T-cycles in Double Speed.
    request.countdown = 4;
    request.source = address;
}

void Dma::tick() {
    if (request.countdown > 0) {
        if (--request.countdown == 0) {
            // Begin DMA transfer
            oam.acquire();
            state = TransferState::Prepare;
            source = request.source;
            cursor = 0;
        }
    }

    if (state == TransferState::Prepare) {
        if (cursor < Specs::MemoryLayout::OAM::SIZE) {
            // Start a read request for the source data
#ifdef ENABLE_CGB
            if (!is_oam_blocked())
#endif
            {
                mmu.read_request(source + cursor);

                // Start a write request to the OAM bus.
                // Note that the request is initiated here: this means that if the PPU
                // will access OAM during the next t0 and t1, conflicts will happen
                // (likely PPU will read from the address written by DMA now).
                // [hacktix/strikethrough]
                oam.write_request(Specs::MemoryLayout::OAM::START + cursor);
            }
            state = TransferState::Flush;
        } else {
            // DMA transfer completed: release OAM bus
            oam.release();
            state = TransferState::None;
        }

    } else if (state == TransferState::Flush) {
        // Complete read request and actually read source data
#ifdef ENABLE_CGB
        if (!is_oam_blocked())
#endif
        {
            const uint8_t src_data = mmu.flush_read_request();

            // Actually write to OAM
            oam.flush_write_request(src_data);
        }

        cursor++;
        state = TransferState::Prepare;
    }
}

void Dma::save_state(Parcel& parcel) const {
    PARCEL_WRITE_UINT8(parcel, request.countdown);
    PARCEL_WRITE_UINT16(parcel, request.source);
    PARCEL_WRITE_UINT8(parcel, state);
    PARCEL_WRITE_UINT16(parcel, source);
    PARCEL_WRITE_UINT8(parcel, cursor);
}

void Dma::load_state(Parcel& parcel) {
    request.countdown = parcel.read_uint8();
    request.source = parcel.read_uint16();
    state = parcel.read_uint8();
    source = parcel.read_uint16();
    cursor = parcel.read_uint8();
}

void Dma::reset() {
    request.countdown = 0;
    request.source = 0;
    state = TransferState::None;
    source = 0;
    cursor = 0;
}

#ifdef ENABLE_CGB
inline bool Dma::is_oam_blocked() const {
    // DMA fails to read or write if a HDMA transfer is in progress
    return oam.is_acquired_by<Device::Hdma>();
}
#endif