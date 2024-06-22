#include "docboy/memory/oam.h"

template <Device::Type Dev>
void OamBus::View<Dev>::read_word_request(uint16_t addr) {
    this->bus.template read_word_request<Dev>(addr);
}

template <Device::Type Dev>
OamBus::Word OamBus::View<Dev>::flush_read_word_request() {
    return this->bus.template flush_read_word_request<Dev>();
}

template <Device::Type Dev>
void OamBus::clear_write_request() {
    reset_bit<W<Dev>>(requests);
}

template <Device::Type Dev>
void OamBus::read_word_request(uint16_t addr) {
    static_assert(Dev == Device::Ppu);

    set_bit<R<Dev>>(requests);

    uint8_t oam_addr = addr - Specs::MemoryLayout::OAM::START;
    if (oam_addr < 0x98) {
        // PPU is reading while CPU is accessing OAM, OAM Bug Corruption happens.
        const auto read_word = [](const UInt8* u) {
            return concat(u[0], u[1]);
        };

        const auto write_word = [](UInt8* u, uint16_t word) {
            u[0] = get_byte<1>(word);
            u[1] = get_byte<0>(word);
        };

        const uint8_t row_addr = discard_bits<3>(oam_addr) + 8 /* next row */;
        UInt8* row = &oam[row_addr];

        if (test_bits_and<R<Device::Cpu>, W<Device::Cpu>>(requests)) {
            if (oam_addr > 0x08) {
                // Read/Write OAM Corruption.
                // row - 16    |  a  |  W1  |  W2  |  W3  |      |  g  |  W5  |  d  |  W7  |
                // row - 8     |  b  |  W5  |  d   |  W7  |  =>  |  g  |  W5  |  d  |  W7  |
                // row         |  c  |  w9  |  W10 |  W11 |      |  g  |  W5  |  d  |  W7  |
                uint16_t a = read_word(row - 16);
                uint16_t b = read_word(row - 8);
                uint16_t c = read_word(row);
                uint16_t d = read_word(row - 4);
                uint16_t g = (b & (a | c | d)) | (a & c & d);
                write_word(row - 8, g);
                for (uint8_t i = 0; i < 4; i++) {
                    uint16_t w = read_word(row - 8 + 2 * i);
                    write_word(row + 2 * i, w);
                    write_word(row - 16 + 2 * i, w);
                }
            } // TODO: else: what happens?
        } else if (test_bit<W<Device::Cpu>>(requests)) {
            // Write OAM Corruption.
            // row - 8     |  b  |  W1  |  c   |  W3  |  =>  |  g  |  W1  |  c   |  W3  |
            // row         |  a  |  W5  |  W6  |  W7  |      |  g  |  W1  |  c   |  W3  |
            uint16_t a = read_word(row);
            uint16_t b = read_word(row - 8);
            uint16_t c = read_word(row - 4);
            uint16_t g = ((a ^ c) & (b ^ c)) ^ c;
            write_word(row, g);
            for (uint8_t i = 1; i < 4; i++) {
                uint16_t w = read_word(row - 8 + 2 * i);
                write_word(row + 2 * i, w);
            }
        } else if (test_bit<R<Device::Cpu>>(requests)) {
            // Read OAM Corruption.
            // row - 8     |  b  |  W1  |  c   |  W3  |  =>  |  g  |  W1  |  c   |  W3  |
            // row         |  a  |  W5  |  W6  |  W7  |      |  g  |  W1  |  c   |  W3  |
            uint16_t a = read_word(row);
            uint16_t b = read_word(row - 8);
            uint16_t c = read_word(row - 4);
            uint16_t g = b | (a & c);
            write_word(row, g);
            for (uint8_t i = 1; i < 4; i++) {
                uint16_t w = read_word(row - 8 + 2 * i);
                write_word(row + i * 2, w);
            }
        }
    }

    // PPU does not overwrite the address in the address bus.
    // i.e. if DMA write is in progress we end up reading from such address instead.
    // [hacktix/strikethrough]
    // TODO: is this actually true also if CPU is writing (i.e. when there's OAM BUG?)
    if (!test_bits_or<W<Device::Dma>, W<Device::Cpu>>(requests)) {
        address = addr;
    }
}

template <Device::Type Dev>
OamBus::Word OamBus::flush_read_word_request() {
    static_assert(Dev == Device::Ppu);

    reset_bit<R<Dev>>(requests);

    // Discard the last bit: base address for read word is always even.
    uint16_t oam_address = discard_bits<0>(address - Specs::MemoryLayout::OAM::START);

    return {oam[oam_address], oam[oam_address + 1]};
}
