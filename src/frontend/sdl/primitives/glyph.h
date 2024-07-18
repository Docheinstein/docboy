#ifndef GLYPH_H
#define GLYPH_H

#include <cstdint>
#include <string>
#include <vector>

constexpr uint8_t GLYPH_WIDTH = 8;
constexpr uint8_t GLYPH_HEIGHT = 8;

using Glyph = uint64_t;

Glyph char_to_glyph(char c);

void draw_glyph(uint32_t* buffer, uint32_t stride, Glyph glyph, uint32_t x, uint32_t y, uint32_t color);

#endif // GLYPH_H