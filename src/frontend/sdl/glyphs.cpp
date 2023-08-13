#include "glyphs.h"

Glyph char_to_glyph(char c) {
    return GLYPHS[static_cast<uint8_t>(c)];
}

void glyph_to_rgba(Glyph glyph, uint32_t color, uint32_t* data, uint32_t start, uint32_t stride) {
    start *= GLYPH_WIDTH;
    stride *= GLYPH_WIDTH;
    for (int r = 0; r < GLYPH_HEIGHT; r++) {
        for (int c = 0; c < GLYPH_WIDTH; c++) {
            auto offset = start + r * stride + c;
            data[offset] = (glyph & 0x8000000000000000) ? color : 0;
            glyph = glyph << 1;
        }
    }
}
