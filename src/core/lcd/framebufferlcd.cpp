#include "framebufferlcd.h"

constexpr uint32_t DARKEST_GREEN_COLOR = 0x144403FF;
constexpr uint32_t DARK_GREEN_COLOR = 0x2B5503FF;
constexpr uint32_t LIGHT_GREEN_COLOR = 0x4D6B03FF;
constexpr uint32_t LIGHTEST_GREEN_COLOR = 0x879603FF;

static uint32_t LCDPixelToRGBA(ILCD::Pixel pixel) {
    switch (pixel) {
    case ILCD::Pixel::Color0:
        return LIGHTEST_GREEN_COLOR;
    case ILCD::Pixel::Color1:
        return LIGHT_GREEN_COLOR;
    case ILCD::Pixel::Color2:
        return DARK_GREEN_COLOR;
    case ILCD::Pixel::Color3:
        return DARKEST_GREEN_COLOR;
    }
    return 0x000000FF;
}

FrameBufferLCD::FrameBufferLCD() : LCD(), pixels() {

}

uint32_t *FrameBufferLCD::getFrameBuffer() {
    return pixels;
}

void FrameBufferLCD::putPixel(ILCD::Pixel pixel, uint8_t x, uint8_t y) {
    pixels[y * Specs::Display::WIDTH + x] = LCDPixelToRGBA(pixel);
}
