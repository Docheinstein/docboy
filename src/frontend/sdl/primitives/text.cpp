#include "primitives/text.h"

#include "primitives/glyph.h"

void draw_text(uint32_t* buffer, uint32_t stride, const std::string& text, uint32_t x, uint32_t y, uint32_t color) {
    const uint32_t length = text.size();
    for (uint32_t i = 0; i < length; i++) {
        draw_glyph(buffer, stride, char_to_glyph(text[i]), x + i * GLYPH_WIDTH, y, color);
    }
}
