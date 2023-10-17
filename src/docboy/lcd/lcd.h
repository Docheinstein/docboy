#ifndef LCD_H
#define LCD_H

#include "docboy/debugger/macros.h"
#include "docboy/shared/specs.h"

class Parcel;

class Lcd {
    DEBUGGABLE_CLASS()

public:
    static constexpr uint16_t PIXEL_COUNT = Specs::Display::WIDTH * Specs::Display::HEIGHT;

    static constexpr uint16_t RGB565_PALETTE[4] {
        0x84A0, // Lightest Green
        0x4B40, // Light Green
        0x2AA0, // Dark Green
        0x1200, // Darkest Green
    };

    using Pixel = uint8_t;
    using PixelRgb565 = uint16_t;

    void reset();

    void pushPixel(Pixel pixel);

    [[nodiscard]] const PixelRgb565* getPixels();

    IF_DEBUGGER(void clearPixels());

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

private:
    PixelRgb565 pixels[PIXEL_COUNT] {};
    uint16_t cursor {};

    IF_DEBUGGER(uint8_t x {});
    IF_DEBUGGER(uint8_t y {});
};

#include "lcd.tpp"

#endif // LCD_H