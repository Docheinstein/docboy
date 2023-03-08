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

