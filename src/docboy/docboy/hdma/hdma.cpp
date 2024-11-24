#include "docboy/hdma/hdma.h"

namespace {
constexpr int TRANSFER_REQUEST_DELAY = 7;
constexpr int REMAINING_CHUNKS_UPDATE_DELAY = 9;
} // namespace

Hdma::Hdma(ExtBus::View<Device::Hdma> ext_bus, VramBus::View<Device::Hdma> vram_bus, const UInt8& stat_mode) :
    ext_bus {ext_bus},
    vram {vram_bus},
    stat_mode {stat_mode} {
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
        ASSERT(mode == TransferMode::HBlank);

        if (!hblank_mode) {
            // Aborts current HDMA transfer
            state = TransferState::None;
        } else {
            // Continue current HDMA transfer
        }
    } else {
        if (hblank_mode) {
            state = TransferState::Paused;
            mode = TransferMode::HBlank;

            if (stat_mode == Specs::Ppu::Modes::HBLANK) {
                // Start instantly
                request_delay = TRANSFER_REQUEST_DELAY;
            }
        } else {
            state = TransferState::None;
            mode = TransferMode::GeneralPurpose;
            request_delay = TRANSFER_REQUEST_DELAY;
        }
    }

    // Note that source and destination cursors are not reset here.
    // They are just reset when writing HDMA2 (source) or HDMA4 (destination).
    // Therefore, writing to HDMA5 without a previous writing to HDMA2/HDMA4 will
    // proceed to transfer accordingly with the current cursors.

    remaining_bytes = 16 * (remaining_chunks.count + 1);

    // TODO: destination outside VRAM
    ASSERT(destination.address >= Specs::MemoryLayout::VRAM::START &&
           destination.address <= Specs::MemoryLayout::VRAM::END);
    ASSERT(remaining_bytes <= 0x800);
}

inline bool Hdma::has_active_transfer() const {
    const bool active = state != TransferState::None || remaining_chunks.delay > 0;
    ASSERT(!active || (remaining_chunks.count != 0xFF));
    return active;
}

void Hdma::tick() {
    if (phase == TransferPhase::Read) {
        if (state == TransferState::Transferring) {
            // Start read request for source data.
            // HDMA reads from EXT bus at the current source address only if the source address
            // is within a valid range, either [0x0000, 0x7FFF] or [0xA000, 0xDFF0].
            // Otherwise, it reads the open bus data from EXT bus.
            const uint16_t source_address = source.address + source.cursor;
            source.valid = source_address < Specs::MemoryLayout::VRAM::START ||
                           (source_address >= Specs::MemoryLayout::RAM::START &&
                            source_address <= Specs::MemoryLayout::WRAM2::END);

            if (source.valid) {
                ext_bus.read_request(source.address + source.cursor);
            }

            // Start write request for VRAM
            vram.write_request(destination.address + destination.cursor);

            phase = TransferPhase::Write;
        }

    } else {
        ASSERT(phase == TransferPhase::Write);

        if (state == TransferState::Transferring) {
            // Complete read request and actually read source data
            const uint8_t src_data = source.valid ? ext_bus.flush_read_request() : ext_bus.open_bus_read();

            // Actually write to VRAM
            vram.flush_write_request(src_data);

            source.cursor++;
            destination.cursor++;

            if (--remaining_bytes == 0) {
                // Transfer completed
                state = TransferState::None;

                // Schedule the update of the remaining chunks (it does not happen immediately)
                remaining_chunks.delay = REMAINING_CHUNKS_UPDATE_DELAY;
                remaining_chunks.scheduled = 0xFF;
            } else if (mode == TransferMode::HBlank) {
                // HDMA pauses each 16 bytes
                if (mod<16>(source.cursor) == 0) {
                    // TODO: can source and dest cursors diverge?
                    //  Don't think so, but write_hdma5 should take it into account for this not to happen.
                    ASSERT(mod<16>(destination.cursor) == 0);

                    // Pause transfer
                    state = TransferState::Paused;

                    // Schedule the update of the remaining chunks (it does not happen immediately)
                    remaining_chunks.delay = REMAINING_CHUNKS_UPDATE_DELAY;
                    remaining_chunks.scheduled = remaining_chunks.count - 1;
                }
            }

            phase = TransferPhase::Read;
        }
    }

    if (state == TransferState::Paused) {
        // Check STAT change
        if (last_stat_mode != Specs::Ppu::Modes::HBLANK && stat_mode == Specs::Ppu::Modes::HBLANK) {
            // Schedule the transfer of the next chunk
            request_delay = TRANSFER_REQUEST_DELAY;
        }
    }

    if (request_delay > 0) {
        // Advance the transfer request state
        if (--request_delay == 0) {
            // Start the transfer
            state = TransferState::Transferring;
        }
    }

    if (remaining_chunks.delay > 0) {
        // Advance the remaining chunks register
        if (--remaining_chunks.delay == 0) {
            // Update the register with the scheduled value
            remaining_chunks.count = remaining_chunks.scheduled;
        }
    }

    last_stat_mode = stat_mode;
}

void Hdma::tick_t0() {
    tick();
}

void Hdma::tick_t1() {
    tick();
}

void Hdma::tick_t2() {
    tick();
}

void Hdma::tick_t3() {
    tick();

    // Update the state of the transfer at the end of each M-Cycle.
    // The CPU will be stalled if a transfer is either active or pending.
    // TODO: this timing works but seems too complex, check further what happens inside.
    active_or_pending_transfer = state == TransferState::Transferring || (request_delay > 0 && request_delay < 4) ||
                                 (remaining_chunks.delay > 7);
}

void Hdma::save_state(Parcel& parcel) const {
    parcel.write_uint8(last_stat_mode);
    parcel.write_uint8(hdma1);
    parcel.write_uint8(hdma2);
    parcel.write_uint8(hdma3);
    parcel.write_uint8(hdma4);
    parcel.write_uint8(active_or_pending_transfer);
    parcel.write_uint8(request_delay);
    parcel.write_uint8(state);
    parcel.write_uint8(mode);
    parcel.write_uint8(phase);
    parcel.write_uint16(source.address);
    parcel.write_uint16(source.cursor);
    parcel.write_bool(source.valid);
    parcel.write_uint16(destination.address);
    parcel.write_uint16(destination.cursor);
    parcel.write_uint16(remaining_bytes);
    parcel.write_uint8(remaining_chunks.count);
    parcel.write_uint8(remaining_chunks.scheduled);
    parcel.write_uint8(remaining_chunks.delay);
}

void Hdma::load_state(Parcel& parcel) {
    last_stat_mode = parcel.read_uint8();
    hdma1 = parcel.read_uint8();
    hdma2 = parcel.read_uint8();
    hdma3 = parcel.read_uint8();
    hdma4 = parcel.read_uint8();
    active_or_pending_transfer = parcel.read_bool();
    request_delay = parcel.read_uint8();
    state = parcel.read_uint8();
    mode = parcel.read_uint8();
    phase = parcel.read_uint8();
    source.address = parcel.read_uint16();
    source.cursor = parcel.read_uint16();
    source.valid = parcel.read_bool();
    destination.address = parcel.read_uint16();
    destination.cursor = parcel.read_uint16();
    remaining_bytes = parcel.read_uint16();
    remaining_chunks.count = parcel.read_uint8();
    remaining_chunks.scheduled = parcel.read_uint8();
    remaining_chunks.delay = parcel.read_uint8();
}

void Hdma::reset() {
    last_stat_mode = 0;
    hdma1 = 0xD4;
    hdma2 = 0x30;
    hdma3 = 0x99;
    hdma4 = 0xD0;
    active_or_pending_transfer = false;
    request_delay = 0;
    state = TransferState::None;
    mode = TransferMode::GeneralPurpose;
    phase = TransferPhase::Read;
    source.address = 0xD430;
    source.cursor = 0;
    source.valid = false;
    destination.address = 0x99D0;
    destination.cursor = 0;
    remaining_bytes = 0;
    remaining_chunks.count = 0xFF;
    remaining_chunks.scheduled = 0xFF;
    remaining_chunks.delay = 0;
}
