#ifndef TEXTURES_H
#define TEXTURES_H

#include <cstdint>

struct SDL_Texture;

uint32_t* lock_texture(SDL_Texture* texture);
void unlock_texture(SDL_Texture* texture);

void clear_texture(uint32_t* buffer, uint32_t size, uint32_t color = 0x00000000);
void copy_to_texture(uint32_t* src, const void* dst, uint32_t size);

#endif // TEXTURES_H
