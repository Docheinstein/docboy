#include "docboy/hdma/hdma.h"

Hdma::Hdma(Mmu::View<Device::Hdma> mmu, VramBus::View<Device::Hdma> vram_bus) :
    mmu {mmu},
    vram {vram_bus} {
}

void Hdma::start_transfer(uint16_t source_address, uint16_t destination_address, uint16_t transfer_length) {
    ASSERT(source_address < Specs::MemoryLayout::VRAM::START ||
           (source_address >= Specs::MemoryLayout::RAM::START && source_address <= Specs::MemoryLayout::WRAM2::END));
    ASSERT(destination_address >= Specs::MemoryLayout::VRAM::START &&
           destination_address <= Specs::MemoryLayout::VRAM::END);
    ASSERT(transfer_length <= 0x800);

    transferring = true;
    source = source_address;
    destination = destination_address;
    length = transfer_length;
    cursor = 0;
}

void Hdma::tick_t0() {
    if (transferring) {
        const uint8_t src_data = mmu.flush_read_request();
        vram.flush_write_request(src_data);
        cursor++;

        if (cursor >= length) {
            transferring = false;
        }
    }
}

void Hdma::tick_t1() {
    if (transferring) {
        mmu.read_request(source + cursor);
        vram.write_request(destination + cursor);
    }
}

void Hdma::tick_t2() {
    if (transferring) {
        const uint8_t src_data = mmu.flush_read_request();
        vram.flush_write_request(src_data);
        cursor++;

        ASSERT(cursor < length);
    }
}

void Hdma::tick_t3() {
    if (transferring) {
        mmu.read_request(source + cursor);
        vram.write_request(destination + cursor);
    }
}

void Hdma::save_state(Parcel& parcel) const {
    parcel.write_bool(transferring);
    parcel.write_uint16(source);
    parcel.write_uint16(destination);
    parcel.write_uint16(length);
    parcel.write_uint16(cursor);
}

void Hdma::load_state(Parcel& parcel) {
    transferring = parcel.read_bool();
    source = parcel.read_uint16();
    destination = parcel.read_uint16();
    length = parcel.read_uint16();
    cursor = parcel.read_uint16();
}

void Hdma::reset() {
    transferring = false;
    source = 0;
    destination = 0;
    length = 0;
    source = 0;
}