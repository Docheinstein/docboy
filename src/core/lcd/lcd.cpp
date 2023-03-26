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

LCD::LCD() : x(), y() {

}

void LCD::putPixel(Pixel pixel, uint8_t x_, uint8_t y_) {
    // nop
}

void LCD::turnOn() {

}

void LCD::turnOff() {
    reset();
}

void LCD::reset() {
    x = 0;
    y = 0;
}

//bool LCD::isOn() const {
//    return get_bit<Bits::LCD::LCDC::LCD_ENABLE>(LCDC);
//}


