#ifndef TEXTURES_H
#define TEXTURES_H

#include <cstdint>

struct SDL_Texture;

void clear_texture(SDL_Texture* texture, uint32_t size, uint32_t color = 0x00000000);
void copy_to_texture(SDL_Texture* texture, const void* src, uint32_t size);

#endif // TEXTURES_H
