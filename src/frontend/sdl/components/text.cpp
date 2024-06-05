#include "components/text.h"

#include "components/glyphs.h"

#include "SDL3/SDL.h"

void draw_text(SDL_Texture* texture, const std::string& text, uint32_t x, uint32_t y, uint32_t color, uint32_t stride) {
    uint32_t* buffer;
    int pitch;

    SDL_LockTexture(texture, nullptr, (void**)&buffer, &pitch);
    draw_text(buffer, text, x, y, color, stride);
    SDL_UnlockTexture(texture);
}

void draw_text(uint32_t* buffer, const std::string& text, uint32_t x, uint32_t y, uint32_t color, uint32_t stride) {
    const uint32_t length = text.size();
    for (uint32_t i = 0; i < length; i++) {
        draw_glyph(buffer, char_to_glyph(text[i]), x + i * GLYPH_WIDTH, y, color, stride);
    }
}
