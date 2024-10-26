#include "docboy/hdma/hdma.h"

Hdma::Hdma(Mmu::View<Device::Hdma> mmu, VramBus::View<Device::Hdma> vram_bus) :
    mmu {mmu},
    vram {vram_bus} {
    reset();
}

void Hdma::write_hdma1(uint8_t value) {
    hdma1 = value;

    // Reload new source address
    source.address = hdma1 << 8 | hdma2;
}

void Hdma::write_hdma2(uint8_t value) {
    hdma2 = discard_bits<4>(value);

    // Reload new source address
    source.address = hdma1 << 8 | hdma2;

    // Reset source cursor
    source.cursor = 0;
}

void Hdma::write_hdma3(uint8_t value) {
    hdma3 = keep_bits<5>(value);

    // Reload new destination address
    destination.address = Specs::MemoryLayout::VRAM::START | (hdma3 << 8 | hdma4);
}

void Hdma::write_hdma4(uint8_t value) {
    hdma4 = discard_bits<4>(value);

    // Reload new destination address
    destination.address = Specs::MemoryLayout::VRAM::START | (hdma3 << 8 | hdma4);

    // Reset destination cursor.
    destination.cursor = 0;
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
            // Continue current HDMA transfer.
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
    }

    // Note that source and destination cursors are not reset here.
    // They are just reset when writing HDMA2 (source) or HDMA4 (destination).
    // Therefore, writing to HDMA5 without a previous writing to HDMA2/HDMA4 will
    // proceed to transfer accordingly with the current cursors.

    transferring = false;
    remaining_bytes = 16 * (remaining_chunks.count + 1);

    // TODO: source in VRAM?
    ASSERT(source.address < Specs::MemoryLayout::VRAM::START ||
           (source.address >= Specs::MemoryLayout::RAM::START && source.address <= Specs::MemoryLayout::WRAM2::END));
    ASSERT(destination.address >= Specs::MemoryLayout::VRAM::START &&
           destination.address <= Specs::MemoryLayout::VRAM::END);
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
        mmu.read_request(source.address + source.cursor);

        // Start write request for VRAM
        vram.write_request(destination.address + destination.cursor);
    }
}

void Hdma::tick_t1() {
    if (transferring) {
        // Complete read request and actually read source data
        const uint8_t src_data = mmu.flush_read_request();

        // Actually write to VRAM
        vram.flush_write_request(src_data);

        source.cursor++;
        destination.cursor++;

        remaining_bytes--;

        ASSERT(mod<2>(source.cursor) == 1);
        ASSERT(mod<2>(destination.cursor) == 1);
        ASSERT(mod<2>(remaining_bytes) == 1);
    }
}

void Hdma::tick_t2() {
    if (transferring) {
        // Start read request for source data
        mmu.read_request(source.address + source.cursor);

        // Start write request for VRAM
        vram.write_request(destination.address + destination.cursor);
    }
}

void Hdma::tick_t3() {
    if (transferring) {
        // Complete read request and actually read source data
        const uint8_t src_data = mmu.flush_read_request();

        // Actually write to VRAM
        vram.flush_write_request(src_data);

        source.cursor++;
        destination.cursor++;

        remaining_bytes--;

        ASSERT(mod<2>(source.cursor) == 0);
        ASSERT(mod<2>(destination.cursor) == 0);
        ASSERT(mod<2>(remaining_bytes) == 0);

        if (remaining_bytes == 0) {
            // Transfer completed
            transferring = false;

            // TODO: maybe this is also scheduled? Anyway it's not observable since CPU is halted in the meantime.
            remaining_chunks.count = 0xFF;
        } else if (mode == TransferMode::HBlank) {
            // HDMA pauses each 16 bytes
            if (mod<16>(source.cursor) == 0) {
                // TODO: source and dest diverge?
                //  Don't think so but write_hdma5 should take it into account for this not to happen.
                ASSERT(mod<16>(destination.cursor) == 0);

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
    parcel.write_uint16(source.address);
    parcel.write_uint16(source.cursor);
    parcel.write_uint16(destination.address);
    parcel.write_uint16(destination.cursor);
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
    source.address = parcel.read_uint16();
    source.cursor = parcel.read_uint16();
    destination.address = parcel.read_uint16();
    destination.cursor = parcel.read_uint16();
    remaining_bytes = parcel.read_uint16();
    remaining_chunks.state = parcel.read_uint8();
    remaining_chunks.count = parcel.read_uint8();
}

void Hdma::reset() {
    hdma1 = 0xD4;
    hdma2 = 0x30;
    hdma3 = 0x99;
    hdma4 = 0xD0;
    request = RequestState::None;
    mode = TransferMode::GeneralPurpose;
    pause = PauseState::None;
    transferring = false;
    source.address = 0xD430;
    source.cursor = 0;
    destination.address = 0x99D0;
    destination.cursor = 0;
    remaining_bytes = 0;
    remaining_chunks.state = RemainingChunksUpdateState::None;
    remaining_chunks.count = 0xFF;
}
