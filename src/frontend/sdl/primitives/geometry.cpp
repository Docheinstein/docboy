#include "primitives/geometry.h"

#include "utils/asserts.h"

namespace {
void draw_horizontal_line(uint32_t* buffer, uint32_t stride, uint32_t a_x, uint32_t b_x, uint32_t y, uint32_t color) {
    for (uint32_t x = a_x; x < b_x; x++) {
        uint32_t offset = y * stride + x;
        buffer[offset] = color;
    }
}

void draw_vertical_line(uint32_t* buffer, uint32_t stride, uint32_t a_y, uint32_t b_y, uint32_t x, uint32_t color) {
    for (uint32_t y = a_y; y < b_y; y++) {
        uint32_t offset = y * stride + x;
        buffer[offset] = color;
    }
}
} // namespace

void draw_line(uint32_t* buffer, uint32_t stride, uint32_t a_x, uint32_t a_y, uint32_t b_x, uint32_t b_y,
               uint32_t color) {
    ASSERT(a_x <= b_x);
    ASSERT(a_y <= b_y);

    if (a_y == b_y) {
        draw_horizontal_line(buffer, stride, a_x, b_x, a_y, color);
    } else if (a_x == b_x) {
        draw_vertical_line(buffer, stride, a_y, b_y, a_x, color);
    } else {
        // Not implemented yet.
        ASSERT_NO_ENTRY();
    }
}

void draw_box(uint32_t* buffer, uint32_t stride, uint32_t min_x, uint32_t min_y, uint32_t max_x, uint32_t max_y,
              uint32_t color) {
    ASSERT(min_x <= max_x);
    ASSERT(min_y <= max_y);

    for (uint32_t y = min_y; y < max_y; y++) {
        for (uint32_t x = min_x; x < max_x; x++) {
            uint32_t offset = y * stride + x;
            buffer[offset] = color;
        }
    }
}
