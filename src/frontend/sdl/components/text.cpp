#include "text.h"
#include "SDL3/SDL.h"
#include "glyphs.h"

void drawText(SDL_Texture* texture, const std::string& text, uint32_t x, uint32_t y, uint32_t color, uint32_t stride) {
    uint32_t* textureBuffer;
    int pitch;

    SDL_LockTexture(texture, nullptr, (void**)&textureBuffer, &pitch);
    drawText(textureBuffer, text, x, y, color, stride);
    SDL_UnlockTexture(texture);
}

void drawText(uint32_t* buffer, const std::string& text, uint32_t x, uint32_t y, uint32_t color, uint32_t stride) {
    const uint32_t stringLength = text.size();
    for (uint32_t i = 0; i < stringLength; i++)
        drawGlyph(buffer, charToGlyph(text[i]), x + i * GLYPH_WIDTH, y, color, stride);
}
