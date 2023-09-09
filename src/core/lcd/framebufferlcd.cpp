#include "framebufferlcd.h"

constexpr uint16_t LCD_PIXEL_TO_RGB565[] {
    0x84A0, // LIGHTEST_GREEN_COLOR
    0x4B40, // LIGHT_GREEN_COLOR
    0x2AA0, // DARK_GREEN_COLOR
    0x1200, // DARKEST_GREEN_COLOR
};

FrameBufferLCD::FrameBufferLCD() :
    LCD(),
    pixels() {
}

uint16_t* FrameBufferLCD::getFrameBuffer() {
    return pixels;
}

void FrameBufferLCD::putPixel(ILCD::Pixel pixel, uint8_t x, uint8_t y) {
    pixels[y * Specs::Display::WIDTH + x] = LCD_PIXEL_TO_RGB565[static_cast<uint8_t>(pixel)];
}
