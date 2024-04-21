#ifndef LCD_H
#define LCD_H

#include <array>
#include <cstring>

#include "docboy/shared//macros.h"
#include "docboy/shared/specs.h"
#include "utils/asserts.h"

class Parcel;

class Lcd {
    DEBUGGABLE_CLASS()

public:
    using Palette = std::array<uint16_t, 4>;
    using Pixel = uint8_t;
    using PixelRgb565 = uint16_t;

    static constexpr uint16_t PIXEL_COUNT = Specs::Display::WIDTH * Specs::Display::HEIGHT;
    static constexpr uint32_t PIXEL_BUFFER_SIZE = PIXEL_COUNT * sizeof(PixelRgb565);

    static constexpr Palette DEFAULT_PALETTE {
        0x84A0, // Lightest Green
        0x4B40, // Light Green
        0x2AA0, // Dark Green
        0x1200, // Darkest Green
    };

    explicit Lcd(const Palette& palette = DEFAULT_PALETTE) :
        palette {palette} {
    }

    void pushPixel(Pixel pixel) {
        check(static_cast<uint8_t>(pixel) < 4);
        pixels[cursor] = palette[static_cast<uint8_t>(pixel)];
        if (++cursor == PIXEL_COUNT)
            cursor = 0;
        IF_DEBUGGER(x = (x + 1) % Specs::Display::WIDTH);
        IF_DEBUGGER(y = (y + (x == 0)) % Specs::Display::HEIGHT);
    }

    [[nodiscard]] const PixelRgb565* getPixels() const {
        return pixels;
    }

    void resetCursor() {
        // Reset the LCD position.
        cursor = 0;
        IF_DEBUGGER(x = 0);
        IF_DEBUGGER(y = 0);
    }

    void clear() {
        // Fills the LCD with color 0.
        for (unsigned short& pixel : pixels)
            pixel = palette[0];
    }

#ifdef ENABLE_DEBUGGER
    Lcd::PixelRgb565* getPixels() {
        return pixels;
    }
    [[nodiscard]] uint16_t getCursor() const {
        return cursor;
    }
#endif

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

    void reset() {
        memset(pixels, 0, sizeof(pixels));
        cursor = 0;
        IF_DEBUGGER(x = 0);
        IF_DEBUGGER(y = 0);
    }

private:
    Palette palette {};

    PixelRgb565 pixels[PIXEL_COUNT] {};
    uint16_t cursor {};

    IF_DEBUGGER(uint8_t x {});
    IF_DEBUGGER(uint8_t y {});
};

#endif // LCD_H