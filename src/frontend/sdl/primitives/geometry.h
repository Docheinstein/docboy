#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <cstdint>

void draw_line(uint32_t* buffer, uint32_t stride, uint32_t a_x, uint32_t a_y, uint32_t b_x, uint32_t b_y,
               uint32_t color);

void draw_box(uint32_t* buffer, uint32_t stride, uint32_t min_x, uint32_t min_y, uint32_t max_x, uint32_t max_y,
              uint32_t color);

#endif // GEOMETRY_H
