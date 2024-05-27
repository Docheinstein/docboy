#ifndef GLYPHS_H
#define GLYPHS_H

#include <cstdint>
#include <string>
#include <vector>

struct SDL_Texture;

constexpr uint8_t GLYPH_WIDTH = 8;
constexpr uint8_t GLYPH_HEIGHT = 8;

using Glyph = uint64_t;

Glyph charToGlyph(char c);

void drawGlyph(SDL_Texture*, Glyph glyph, uint32_t x, uint32_t y, uint32_t color, uint32_t stride);
void drawGlyph(uint32_t* buffer, Glyph glyph, uint32_t x, uint32_t y, uint32_t color, uint32_t stride);

#endif // GLYPHS_H