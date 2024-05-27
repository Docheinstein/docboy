#ifndef TEXT_H
#define TEXT_H

#include <cstdint>
#include <string>

struct SDL_Texture;

void drawText(SDL_Texture* texture, const std::string& text, uint32_t x, uint32_t y, uint32_t color, uint32_t stride);
void drawText(uint32_t* buffer, const std::string& text, uint32_t x, uint32_t y, uint32_t color, uint32_t stride);

#endif // TEXT_H
