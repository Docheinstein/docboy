#include "docboy/hdma/hdma.h"

Hdma::Hdma(Mmu::View<Device::Hdma> mmu, VramBus::View<Device::Hdma> vram_bus) :
    mmu {mmu},
    vram {vram_bus} {
    reset();
}

void Hdma::write_hdma1(uint8_t value) {
    hdma1 = value;
}

void Hdma::write_hdma2(uint8_t value) {
    hdma2 = discard_bits<4>(value);
}

void Hdma::write_hdma3(uint8_t value) {
    hdma3 = keep_bits<5>(value);
}

void Hdma::write_hdma4(uint8_t value) {
    hdma4 = discard_bits<4>(value);
}

uint8_t Hdma::read_hdma5() const {
    if (!transferring && pause == PauseState::None) {
        // Neither transferring nor in pause: just reads FF
        return 0xFF;
    }

    return remaining_chunks.count - 1;
}

void Hdma::write_hdma5(uint8_t value) {
    hdma5.hblank_mode = test_bit<Specs::Bits::Hdma::HDMA5::HBLANK_TRANSFER>(value);
    hdma5.length = get_bits_range<Specs::Bits::Hdma::HDMA5::TRANSFER_LENGTH>(value); /* TODO: probably can be omitted */

    transferring = false;

    source = hdma1 << 8 | hdma2;
    destination = Specs::MemoryLayout::VRAM::START | (hdma3 << 8 | hdma4);
    remaining_chunks.count = hdma5.length + 1;
    length = 16 * remaining_chunks.count;
    cursor = 0;

    if (hdma5.hblank_mode) {
        request = RequestState::None;
        mode = TransferMode::HBlank;
        pause = PauseState::Paused;
    } else {
        request = RequestState::Requested;
        mode = TransferMode::GeneralPurpose;
        pause = PauseState::None;
    }

    ASSERT(source < Specs::MemoryLayout::VRAM::START ||
           (source >= Specs::MemoryLayout::RAM::START && source <= Specs::MemoryLayout::WRAM2::END));
    ASSERT(destination >= Specs::MemoryLayout::VRAM::START && destination <= Specs::MemoryLayout::VRAM::END);
    ASSERT(length <= 0x800);
}

void Hdma::resume() {
    if (pause == PauseState::Paused) {
        // Schedule the resume of the transfer
        pause = PauseState::ResumeRequested;
    }
}

void Hdma::tick_t0() {
    if (transferring) {
        // Start read request for source data
        mmu.read_request(source + cursor);

        // Start write request for VRAM
        vram.write_request(destination + cursor);
    }
}

void Hdma::tick_t1() {
    if (transferring) {
        // Complete read request and actually read source data
        const uint8_t src_data = mmu.flush_read_request();

        // Actually write to VRAM
        vram.flush_write_request(src_data);

        cursor++;

        ASSERT(mod<2>(cursor) == 1);
    }
}

void Hdma::tick_t2() {
    if (transferring) {
        // Start read request for source data
        mmu.read_request(source + cursor);

        // Start write request for VRAM
        vram.write_request(destination + cursor);
    }
}

void Hdma::tick_t3() {
    if (transferring) {
        // Complete read request and actually read source data
        const uint8_t src_data = mmu.flush_read_request();

        // Actually write to VRAM
        vram.flush_write_request(src_data);

        cursor++;

        ASSERT(mod<2>(cursor) == 0);

        if (cursor >= length) {
            // Transfer completed
            transferring = false;
        } else if (mode == TransferMode::HBlank) {
            // HDMA pauses each 16 bytes
            if (mod<16>(cursor) == 0) {
                // Pause transfer
                transferring = false;
                pause = PauseState::Paused;

                // Schedule the update of the remaining chunks register
                remaining_chunks.state = RemainingChunksUpdateState::Requested;
            }
        }
    } else {
        if (request != RequestState::None) {
            // Advance the transfer request state
            if (--request == RequestState::None) {
                transferring = true;
            }
        }

        if (pause > PauseState::None) {
            // Advance the resume request state
            if (--pause == PauseState::None) {
                transferring = true;
            }
        }

        if (remaining_chunks.state > RemainingChunksUpdateState::None) {
            // Advance the remaining chunks register
            if (--remaining_chunks.state == RemainingChunksUpdateState::None) {
                --remaining_chunks.count;
            }
        }
    }
}

void Hdma::save_state(Parcel& parcel) const {
    parcel.write_uint8(hdma1);
    parcel.write_uint8(hdma2);
    parcel.write_uint8(hdma3);
    parcel.write_uint8(hdma4);
    parcel.write_bool(hdma5.hblank_mode);
    parcel.write_uint8(hdma5.length);
    parcel.write_uint8(request);
    parcel.write_uint8(mode);
    parcel.write_uint8(pause);
    parcel.write_bool(transferring);
    parcel.write_uint16(source);
    parcel.write_uint16(destination);
    parcel.write_uint16(length);
    parcel.write_uint16(cursor);
    parcel.write_uint8(remaining_chunks.state);
    parcel.write_uint8(remaining_chunks.count);
}

void Hdma::load_state(Parcel& parcel) {
    hdma1 = parcel.read_uint8();
    hdma2 = parcel.read_uint8();
    hdma3 = parcel.read_uint8();
    hdma4 = parcel.read_uint8();
    hdma5.hblank_mode = parcel.read_bool();
    hdma5.length = parcel.read_uint8();
    request = parcel.read_uint8();
    mode = parcel.read_uint8();
    pause = parcel.read_uint8();
    transferring = parcel.read_bool();
    source = parcel.read_uint16();
    destination = parcel.read_uint16();
    length = parcel.read_uint16();
    cursor = parcel.read_uint16();
    remaining_chunks.state = parcel.read_uint8();
    remaining_chunks.count = parcel.read_uint8();
}

void Hdma::reset() {
    hdma1 = 0;
    hdma2 = 0;
    hdma3 = 0;
    hdma4 = 0;
    hdma5.hblank_mode = false;
    hdma5.length = 0;
    request = RequestState::None;
    mode = TransferMode::GeneralPurpose;
    pause = PauseState::None;
    transferring = false;
    source = 0;
    destination = 0;
    length = 0;
    cursor = 0;
    remaining_chunks.state = RemainingChunksUpdateState::None;
    remaining_chunks.count = 0;
}
