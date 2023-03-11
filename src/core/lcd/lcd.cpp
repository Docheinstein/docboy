#include "lcd.h"

void LCD::pushPixel(ILCD::Pixel pixel) {
    putPixel(pixel, x, y);
    x++;
    if (x >= 160) {
        x = 0;
        y++;
    }
    if (y >= 144) {
        x = 0;
        y = 0;
    }
}

LCD::LCD() : on(), x(), y() {

}

bool LCD::isOn() const {
    return on;
}

void LCD::turnOn() {
    on = true;
}

void LCD::turnOff() {
    // TODO: not clean, maybe x, y should be handled by PPU entirely?
    x = 0;
    y = 0;
    on = false;
}

void LCD::putPixel(Pixel pixel, uint8_t x_, uint8_t y_) {
    // nop
}
