#include "docboy/hdma/hdma.h"
#include "docboy/dma/dma.h"
#include "docboy/speedswitch/speedswitchcontroller.h"

Hdma::Hdma(Mmu::View<Device::Hdma> mmu, ExtBus::View<Device::Hdma> ext_bus, VramBus::View<Device::Hdma> vram_bus,
           OamBus::View<Device::Hdma> oam_bus, const UInt8& stat_mode, const bool& fetching, const bool& halted,
           const bool& stopped, SpeedSwitchController& speed_switch_controller) :
    mmu {mmu},
    ext_bus {ext_bus},
    vram {vram_bus},
    oam {oam_bus},
    stat_mode {stat_mode},
    fetching {fetching},
    halted {halted},
    stopped {stopped},
    speed_switch_controller {speed_switch_controller} {
    reset();
}

void Hdma::write_hdma1(uint8_t value) {
    hdma1 = value;

    // Reload new source address
    source.address = concat(hdma1, hdma2);
}

void Hdma::write_hdma2(uint8_t value) {
    hdma2 = discard_bits<4>(value);

    // Reload new source address
    source.address = concat(hdma1, hdma2);

    // Reset source cursor
    source.cursor = 0;
}

void Hdma::write_hdma3(uint8_t value) {
    // Note: all the bits are written to HDMA3, not only the lowest 5.
    // This is shown by the fact that writing 0xFF00 has a different behavior
    // compared to writing, for example 0x9F00. Indeed, the first one will
    // make destination address reach 0xFFFF after the first 256 bytes, which will
    // abort the transfer, while the latter will overflow several times (module 0x2000)
    // before reach 0xFFFF.
    hdma3 = value;

    // Reload new destination address
    destination.address = Specs::MemoryLayout::VRAM::START | concat(hdma3, hdma4);
}

void Hdma::write_hdma4(uint8_t value) {
    hdma4 = discard_bits<4>(value);

    // Reload new destination address
    destination.address = Specs::MemoryLayout::VRAM::START | concat(hdma3, hdma4);

    // Reset destination cursor
    destination.cursor = 0;
}

uint8_t Hdma::read_hdma5() const {
    return (state != TransferState::None ? 0x00 : 0x80) | keep_bits<7>(remaining_chunks.count);
}

void Hdma::write_hdma5(uint8_t value) {
    const bool hblank_mode = test_bit<Specs::Bits::Hdma::HDMA5::HBLANK_TRANSFER>(value);
    remaining_chunks.count = get_bits_range<Specs::Bits::Hdma::HDMA5::TRANSFER_LENGTH>(value);

    remaining_bytes = 16 * (remaining_chunks.count + 1);
    ASSERT(remaining_bytes <= 0x800);

    if (state != TransferState::None) {
        ASSERT(mode == TransferMode::HBlank);

        if (!hblank_mode) {
            // Aborts current HDMA transfer
            state = TransferState::None;
        }
    } else {
        if (hblank_mode) {
            mode = TransferMode::HBlank;

            // Transfer is started instantly if STAT's mode is HBlank
            state = stat_mode == Specs::Ppu::Modes::HBLANK ? TransferState::Requested : TransferState::Paused;
        } else {
            mode = TransferMode::GeneralPurpose;
            state = TransferState::Requested;
        }
    }

    // Note that source and destination cursors are not reset here.
    // They are just reset when writing HDMA2 (source) or HDMA4 (destination).
    // Therefore, writing to HDMA5 without a previous writing to HDMA2/HDMA4 will
    // proceed to transfer accordingly with the current cursors.
}

void Hdma::tick_t0() {
    tick_even();
    tick_state();
    tick_request();
}

void Hdma::tick_t1() {
    tick_odd();
    tick_state();
    if (speed_switch_controller.is_double_speed_mode()) {
        tick_phase();
    }
}

void Hdma::tick_t2() {
    tick_even();
    tick_state();
    tick_request();
}

void Hdma::tick_t3() {
    tick_odd();
    tick_state();
    tick_phase();
}

void Hdma::tick_even() {
    if (state == TransferState::Transferring) {
        // Start read request for source data.
        // HDMA reads real data only if the source address is within a valid range,
        // either [0x0000, 0x7FFF] or [0xA000, 0xDFF0].
        // Otherwise, it reads the open bus data from EXT bus.
        const uint16_t source_address = source.address + source.cursor;
        source.valid =
            source_address < Specs::MemoryLayout::VRAM::START ||
            (source_address >= Specs::MemoryLayout::RAM::START && source_address <= Specs::MemoryLayout::WRAM2::END);

        if (source.valid) {
            mmu.read_request(source_address);
        }

        // If DMA is transferring concurrently with HDMA, the byte HDMA is writing will be written
        // to OAM too, at the address given by the modulo 256 of the current HDMA source address.
        // This seems to happen once each two bytes.
        dma_oam_conflict = oam.is_acquired_by<Device::Dma>() && mod<2>(source.cursor) == 1;

        if (dma_oam_conflict) {
            oam.write_request(Specs::MemoryLayout::OAM::START + mod<256>(source_address));
        }

        // Start write request for VRAM.
        // Destination address overflows with modulo 0x2000.
        // HDMA is aborted only if the destination exceeds 0xFFFF.
        const uint16_t destination_address_slack = keep_bits<13>(destination.address + destination.cursor);
        const uint16_t destination_address = Specs::MemoryLayout::VRAM::START | destination_address_slack;
        ASSERT(destination_address >= Specs::MemoryLayout::VRAM::START &&
               destination_address <= Specs::MemoryLayout::VRAM::END);
        vram.write_request(destination_address);
    }
}

void Hdma::tick_odd() {
    if (state == TransferState::Transferring) {
        // Complete read request and actually read source data
        const uint8_t src_data = source.valid ? mmu.flush_read_request() : ext_bus.open_bus_read();

        // Actually write to VRAM
        vram.flush_write_request(src_data);

        // Eventually write to OAM too if DMA is currently transferring
        if (dma_oam_conflict) {
            oam.flush_write_request(src_data);
        }

        // Advance cursors
        source.cursor++;
        destination.cursor++;
        remaining_bytes--;

        if (mod<16>(remaining_bytes) == 0) {
            // A chunk of 16 bytes has been completed

            ASSERT(mod<16>(source.cursor) == 0);
            ASSERT(mod<16>(destination.cursor) == 0);

            if (remaining_bytes == 0) {
                // Transfer completed
                state = TransferState::None;
            } else if (mode == TransferMode::HBlank) {
                // HBlank HDMA transfer is paused at the completion of a 16 bytes chunk
                state = TransferState::Paused;
            }

            if ((uint16_t)(destination.address + destination.cursor)) {
                if (remaining_bytes == 0) {
                    // Transfer completed
                    state = TransferState::None;
                } else if (mode == TransferMode::HBlank) {
                    // HBlank HDMA transfer is paused at the completion of a 16 bytes chunk
                    state = TransferState::Paused;
                }

                if (mode == Hdma::TransferMode::HBlank || remaining_chunks.count == 0) {
                    // Schedule the unblock of the CPU if all the chunks have been transferred,
                    // or after every chunk anyway in HBlank mode.
                    unblock = UnblockState::Requested;
                }
            } else {
                // Destination address overflowed 0xFFFF.
                // The transfer is stopped and the destination is reset to 0x8000.
                state = TransferState::None;
                remaining_bytes = 0;
                hdma3 = 0x00;
                hdma4 = 0x00;
                destination.address = Specs::MemoryLayout::VRAM::START;
                destination.cursor = 0;
                unblock = UnblockState::Requested;
            }

            if (state != TransferState::Transferring) {
                // Release buses
                oam.release();
                vram.release();
            }

            // Schedule the update of the remaining chunks
            remaining_chunks.state = RemainingChunksUpdateState::Requested;
        }
    }
}

void Hdma::tick_state() {
    if (state == TransferState::Paused) {
        // Check STAT change.
        // Request is ignored if GB is either halted or stopped.
        // It seems that HDMA sees the HALT state with a delay of 1 T-Cycle.
        // Whereas STOP timing seems to be non deterministic.
        // TODO: further investigations.
        if (last_stat_mode != Specs::Ppu::Modes::HBLANK && stat_mode == Specs::Ppu::Modes::HBLANK && !last_halted &&
            !stopped) {
            // Schedule the transfer of the next chunk
            state = TransferState::Requested;
        }
    }

    last_stat_mode = stat_mode;

    // It seems that there is window of 1 T-Cycle that HDMA HBlank can start before GB is actually halted
    last_halted = halted;
}

void Hdma::tick_request() {
    // HDMA is actually ready to start after two T0 or T2 elapsed after the trigger
    // (that could be either a writing to HDMA5, or STAT's mode changing to HBlank, in HBlank mode).
    if (state == TransferState::Requested) {
        state = TransferState::Pending;
    } else if (state == TransferState::Pending) {
        state = TransferState::Ready;
    }
}

void Hdma::tick_phase() {
    if (state == TransferState::Ready) {
        if (fetching) {
            // Actually start the HDMA if is ready (at least a T0 is passed from the trigger)
            // and this is the last micro-operation of the CPU instruction.
            state = Hdma::TransferState::Transferring;
            active = true;

            // Acquire video buses
            oam.acquire();
            vram.acquire();
        }
    } else {
        if (remaining_chunks.state == RemainingChunksUpdateState::Requested) {
            ASSERT(state == TransferState::None || state == TransferState::Paused ||
                   state == TransferState::Transferring);

            // Decrease the remaining chunk count
            remaining_chunks.state = RemainingChunksUpdateState::None;
            remaining_chunks.count--;
        } else if (unblock != UnblockState::None) {
            if (--unblock == UnblockState::None) {
                ASSERT(state == TransferState::None || state == TransferState::Paused);
                ASSERT(active);

                // Unblock the CPU
                unblock = UnblockState::None;
                active = false;
            }
        }
    }
}

void Hdma::save_state(Parcel& parcel) const {
    parcel.write_uint8(hdma1);
    parcel.write_uint8(hdma2);
    parcel.write_uint8(hdma3);
    parcel.write_uint8(hdma4);
    parcel.write_uint8(last_stat_mode);
    parcel.write_bool(last_halted);
    parcel.write_bool(active);
    parcel.write_uint8(state);
    parcel.write_uint8(mode);
    parcel.write_uint16(source.address);
    parcel.write_uint16(source.cursor);
    parcel.write_bool(source.valid);
    parcel.write_uint16(destination.address);
    parcel.write_uint16(destination.cursor);
    parcel.write_uint16(remaining_bytes);
    parcel.write_uint8(remaining_chunks.state);
    parcel.write_uint8(remaining_chunks.count);
    parcel.write_uint8(unblock);
    parcel.write_bool(dma_oam_conflict);
}

void Hdma::load_state(Parcel& parcel) {
    hdma1 = parcel.read_uint8();
    hdma2 = parcel.read_uint8();
    hdma3 = parcel.read_uint8();
    hdma4 = parcel.read_uint8();
    last_stat_mode = parcel.read_uint8();
    last_halted = parcel.read_bool();
    active = parcel.read_bool();
    state = parcel.read_uint8();
    mode = parcel.read_uint8();
    source.address = parcel.read_uint16();
    source.cursor = parcel.read_uint16();
    source.valid = parcel.read_bool();
    destination.address = parcel.read_uint16();
    destination.cursor = parcel.read_uint16();
    remaining_bytes = parcel.read_uint16();
    remaining_chunks.state = parcel.read_uint8();
    remaining_chunks.count = parcel.read_uint8();
    unblock = parcel.read_uint8();
    dma_oam_conflict = parcel.read_bool();
}

void Hdma::reset() {
    hdma1 = 0xD4;
    hdma2 = 0x30;
    hdma3 = 0x99;
    hdma4 = 0xD0;
    last_stat_mode = 0;
    last_halted = false;
    active = false;
    state = TransferState::None;
    mode = TransferMode::GeneralPurpose;
    source.address = 0xD430;
    source.cursor = 0;
    source.valid = false;
    destination.address = 0x99D0;
    destination.cursor = 0;
    remaining_bytes = 0;
    remaining_chunks.state = RemainingChunksUpdateState::None;
    remaining_chunks.count = 0xFF;
    unblock = UnblockState::None;
    dma_oam_conflict = false;
}
