#include "sdllcd.h"

constexpr uint32_t DARKEST_GREEN_COLOR = 0x0f380fff;
constexpr uint32_t DARK_GREEN_COLOR = 0x306230ff;
constexpr uint32_t LIGHT_GREEN_COLOR = 0x8bac0fff;
constexpr uint32_t LIGHTEST_GREEN_COLOR = 0x9bbc0fff;

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

SDLLCD::SDLLCD() : on(), pixels() {

}

void SDLLCD::putPixel(ILCD::Pixel pixel, uint8_t x, uint8_t y) {
    pixels[y * Specs::Display::WIDTH + x] = LCDPixelToRGBA(pixel);
}

bool SDLLCD::isOn() const {
    return on;
}

void SDLLCD::turnOn() {
    on = true;
}

void SDLLCD::turnOff() {
    on = false;
}


uint32_t *SDLLCD::getFrameBuffer() {
    return pixels;
}
