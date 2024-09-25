#ifndef TEXT_H
#define TEXT_H

#include <cstdint>
#include <string>

void draw_text(uint32_t* buffer, uint32_t stride, const std::string& text, uint32_t x, uint32_t y, uint32_t color);

#endif // TEXT_H
