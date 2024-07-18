#include "utils/textures.h"

#include "SDL3/SDL.h"

uint32_t* lock_texture(SDL_Texture* texture) {
    uint32_t* buffer;
    int pitch;

    SDL_LockTexture(texture, nullptr, (void**)&buffer, &pitch);

    return buffer;
}

void unlock_texture(SDL_Texture* texture) {
    SDL_UnlockTexture(texture);
}

void clear_texture(uint32_t* buffer, uint32_t size, uint32_t color) {
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = color;
    }
}

void copy_to_texture(uint32_t* dst, const void* src, uint32_t size) {
    memcpy(dst, src, size);
}