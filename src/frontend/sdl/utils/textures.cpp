#include "utils/textures.h"

#include "SDL3/SDL.h"

void clear_texture(SDL_Texture* texture, uint32_t size, uint32_t color) {
    uint32_t* buffer;
    int pitch;

    SDL_LockTexture(texture, nullptr, (void**)&buffer, &pitch);
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = color;
    }
    SDL_UnlockTexture(texture);
}

void copy_to_texture(SDL_Texture* texture, const void* src, uint32_t size) {
    void* buffer;
    int pitch;

    SDL_LockTexture(texture, nullptr, (void**)&buffer, &pitch);
    memcpy(buffer, src, size);
    SDL_UnlockTexture(texture);
}
