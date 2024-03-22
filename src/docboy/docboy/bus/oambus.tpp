template <Device::Type Dev>
void OamBus::clearWriteRequest() {
    reset_bit<W<Dev>>(requests);
}

template <Device::Type Dev>
void OamBus::readWordRequest(uint16_t addr) {
    static_assert(Dev == Device::Ppu);

    set_bit<R<Dev>>(requests);

    uint8_t oamAddr = addr - Specs::MemoryLayout::OAM::START;
    if (oamAddr < 0x98) {
        // PPU is reading while CPU is accessing OAM, OAM Bug Corruption happens.
        const auto readWord = [](const byte* b) {
            return concat(b[0], b[1]);
        };

        const auto writeWord = [](byte* b, uint16_t word) {
            b[0] = get_byte<1>(word);
            b[1] = get_byte<0>(word);
        };

        const uint8_t rowAddr = discard_bits<3>(oamAddr) + 8 /* next row */;
        byte* row = &oam[rowAddr];

        if (test_bits_and<R<Device::Cpu>, W<Device::Cpu>>(requests)) {
            if (oamAddr > 0x08) {
                // Read/Write OAM Corruption.
                // row - 16    |  a  |  W1  |  W2  |  W3  |      |  g  |  W5  |  d  |  W7  |
                // row - 8     |  b  |  W5  |  d   |  W7  |  =>  |  g  |  W5  |  d  |  W7  |
                // row         |  c  |  w9  |  W10 |  W11 |      |  g  |  W5  |  d  |  W7  |
                uint16_t a = readWord(row - 16);
                uint16_t b = readWord(row - 8);
                uint16_t c = readWord(row);
                uint16_t d = readWord(row - 4);
                uint16_t g = (b & (a | c | d)) | (a & c & d);
                writeWord(row - 8, g);
                for (uint8_t i = 0; i < 4; i++) {
                    uint16_t w = readWord(row - 8 + 2 * i);
                    writeWord(row + 2 * i, w);
                    writeWord(row - 16 + 2 * i, w);
                }
            } // TODO: else: what happens?
        } else if (test_bit<W<Device::Cpu>>(requests)) {
            // Write OAM Corruption.
            // row - 8     |  b  |  W1  |  c   |  W3  |  =>  |  g  |  W1  |  c   |  W3  |
            // row         |  a  |  W5  |  W6  |  W7  |      |  g  |  W1  |  c   |  W3  |
            uint16_t a = readWord(row);
            uint16_t b = readWord(row - 8);
            uint16_t c = readWord(row - 4);
            uint16_t g = ((a ^ c) & (b ^ c)) ^ c;
            writeWord(row, g);
            for (uint8_t i = 1; i < 4; i++) {
                uint16_t w = readWord(row - 8 + 2 * i);
                writeWord(row + 2 * i, w);
            }
        } else if (test_bit<R<Device::Cpu>>(requests)) {
            // Read OAM Corruption.
            // row - 8     |  b  |  W1  |  c   |  W3  |  =>  |  g  |  W1  |  c   |  W3  |
            // row         |  a  |  W5  |  W6  |  W7  |      |  g  |  W1  |  c   |  W3  |
            uint16_t a = readWord(row);
            uint16_t b = readWord(row - 8);
            uint16_t c = readWord(row - 4);
            uint16_t g = b | (a & c);
            writeWord(row, g);
            for (uint8_t i = 1; i < 4; i++) {
                uint16_t w = readWord(row - 8 + 2 * i);
                writeWord(row + i * 2, w);
            }
        }
    }

    // PPU does not overwrite the address in the address bus.
    // i.e. if DMA write is in progress we end up reading from such address instead.
    // [hacktix/strikethrough]
    // TODO: is this actually true also if CPU is writing (i.e. when there's OAM BUG?)
    if (!test_bits_or<W<Device::Dma>, W<Device::Cpu>>(requests))
        address = addr;
}

template <Device::Type Dev>
OamBus::Word OamBus::flushReadWordRequest() {
    static_assert(Dev == Device::Ppu);

    reset_bit<R<Dev>>(requests);

    // Discard the last bit: base address for read word is always even.
    uint16_t oamAddress = discard_bits<0>(address - Specs::MemoryLayout::OAM::START);

    return {oam[oamAddress], oam[oamAddress + 1]};
}
