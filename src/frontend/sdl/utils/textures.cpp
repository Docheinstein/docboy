#include "textures.h"
#include "SDL3/SDL.h"

void clearTexture(SDL_Texture* texture, uint32_t size, uint32_t color) {
    uint32_t* textureBuffer;
    int pitch;

    SDL_LockTexture(texture, nullptr, (void**)&textureBuffer, &pitch);
    for (uint32_t i = 0; i < size; i++)
        textureBuffer[i] = color;
    SDL_UnlockTexture(texture);
}

void copyToTexture(SDL_Texture* texture, const void* src, uint32_t size) {
    void* textureBuffer;
    int pitch;

    SDL_LockTexture(texture, nullptr, (void**)&textureBuffer, &pitch);
    memcpy(textureBuffer, src, size);
    SDL_UnlockTexture(texture);
}
