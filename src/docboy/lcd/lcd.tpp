#include "lcd.h"
#include "utils/asserts.h"
#include <cstring>

inline void Lcd::reset() {
    cursor = 0;
    IF_DEBUGGER(x = 0);
    IF_DEBUGGER(y = 0);
}

inline void Lcd::pushPixel(Lcd::Pixel pixel) {
    check(static_cast<uint8_t>(pixel) < 4);
    pixels[cursor] = RGB565_PALETTE[static_cast<uint8_t>(pixel)];
    if (++cursor == PIXEL_COUNT)
        cursor = 0;
    IF_DEBUGGER(x = (x + 1) % Specs::Display::WIDTH);
    IF_DEBUGGER(y = (y + (x == 0)) % Specs::Display::HEIGHT);
}

inline const Lcd::PixelRgb565 *Lcd::getPixels() {
    return pixels;
}


#ifdef ENABLE_DEBUGGER
inline Lcd::PixelRgb565* Lcd::getMutablePixels() {
    return pixels;
}
inline uint16_t Lcd::getCursor() const {
    return cursor;
}
#endif