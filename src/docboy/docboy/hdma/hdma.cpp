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
    return (has_active_transfer() ? 0x00 : 0x80) | keep_bits<7>(remaining_chunks.count);
}

void Hdma::write_hdma5(uint8_t value) {
    const bool hblank_mode = test_bit<Specs::Bits::Hdma::HDMA5::HBLANK_TRANSFER>(value);
    remaining_chunks.count = get_bits_range<Specs::Bits::Hdma::HDMA5::TRANSFER_LENGTH>(value);

    if (has_active_transfer()) {
        ASSERT(request == RequestState::None);
        ASSERT(mode == TransferMode::HBlank);

        if (!hblank_mode) {
            // Aborts current HDMA transfer
            pause = PauseState::None;
        } else {
            // Continue current HDMA transfer, but update the remaining amount of bytes to transfer.
            // Note that the cursor is not reset in this case, therefore the transfer will continue
            // from the address it was.
        }
    } else {
        if (hblank_mode) {
            request = RequestState::None;
            mode = TransferMode::HBlank;
            pause = PauseState::Paused;
        } else {
            request = RequestState::Requested;
            mode = TransferMode::GeneralPurpose;
            pause = PauseState::None;
        }

        source = hdma1 << 8 | hdma2;
        destination = Specs::MemoryLayout::VRAM::START | (hdma3 << 8 | hdma4);
        cursor = 0;
    }

    transferring = false;
    remaining_bytes = 16 * (remaining_chunks.count + 1);

    ASSERT(source < Specs::MemoryLayout::VRAM::START ||
           (source >= Specs::MemoryLayout::RAM::START && source <= Specs::MemoryLayout::WRAM2::END));
    ASSERT(destination >= Specs::MemoryLayout::VRAM::START && destination <= Specs::MemoryLayout::VRAM::END);
    ASSERT(remaining_bytes <= 0x800);
}

void Hdma::resume() {
    if (pause == PauseState::Paused) {
        // Schedule the resume of the transfer
        pause = PauseState::ResumeRequested;
    }
}

inline bool Hdma::has_active_transfer() const {
    return transferring || pause != PauseState::None;
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
        remaining_bytes--;

        ASSERT(mod<2>(cursor) == 1);
        ASSERT(mod<2>(remaining_bytes) == 1);
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
        remaining_bytes--;

        ASSERT(mod<2>(cursor) == 0);
        ASSERT(mod<2>(remaining_bytes) == 0);

        if (remaining_bytes == 0) {
            // Transfer completed
            transferring = false;

            // TODO: maybe this is also scheduled? Anyway it's not observable since CPU is halted in the meantime.
            remaining_chunks.count = 0xFF;
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
    parcel.write_uint8(request);
    parcel.write_uint8(mode);
    parcel.write_uint8(pause);
    parcel.write_bool(transferring);
    parcel.write_uint16(source);
    parcel.write_uint16(destination);
    parcel.write_uint16(cursor);
    parcel.write_uint16(remaining_bytes);
    parcel.write_uint8(remaining_chunks.state);
    parcel.write_uint8(remaining_chunks.count);
}

void Hdma::load_state(Parcel& parcel) {
    hdma1 = parcel.read_uint8();
    hdma2 = parcel.read_uint8();
    hdma3 = parcel.read_uint8();
    hdma4 = parcel.read_uint8();
    request = parcel.read_uint8();
    mode = parcel.read_uint8();
    pause = parcel.read_uint8();
    transferring = parcel.read_bool();
    source = parcel.read_uint16();
    destination = parcel.read_uint16();
    cursor = parcel.read_uint16();
    remaining_bytes = parcel.read_uint16();
    remaining_chunks.state = parcel.read_uint8();
    remaining_chunks.count = parcel.read_uint8();
}

void Hdma::reset() {
    hdma1 = 0;
    hdma2 = 0;
    hdma3 = 0;
    hdma4 = 0;
    request = RequestState::None;
    mode = TransferMode::GeneralPurpose;
    pause = PauseState::None;
    transferring = false;
    source = 0;
    destination = 0;
    cursor = 0;
    remaining_bytes = 0;
    remaining_chunks.state = RemainingChunksUpdateState::None;
    remaining_chunks.count = 0xFF;
}
