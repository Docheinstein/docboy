#ifndef LCD_H
#define LCD_H

#include <array>
#include <cstring>

#include "docboy/common/macros.h"
#include "docboy/common/specs.h"
#include "docboy/lcd/appearance.h"

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"

class Parcel;

class Lcd {
    DEBUGGABLE_CLASS()

public:
#ifdef ENABLE_CGB
    using Pixel = uint16_t /* [0:32767]*/;
#else
    using Pixel = uint8_t /* [0:3] */;
#endif

    static constexpr uint16_t PIXEL_COUNT = Specs::Display::WIDTH * Specs::Display::HEIGHT;
    static constexpr uint32_t PIXEL_BUFFER_SIZE = PIXEL_COUNT * sizeof(PixelRgb565);

    Lcd();

    void set_appearance(const Appearance& a);

    void push_pixel(Pixel pixel) {
        ASSERT(pixel < Appearance::NUM_COLORS);
        pixels[cursor] = appearance.palette[pixel];
        if (++cursor == PIXEL_COUNT) {
            cursor = 0;
        }
#ifdef ENABLE_DEBUGGER
        x = (x + 1) % Specs::Display::WIDTH;
        y = (y + (x == 0)) % Specs::Display::HEIGHT;
#endif
    }

    const PixelRgb565* get_pixels() const {
        return pixels;
    }

    void clear() {
        // Fills the LCD with the default color
        for (uint32_t i = 0; i < array_size(pixels); ++i) {
            pixels[i] = appearance.default_color;
        }
    }

    void rewind() {
        // Reset the LCD position
        cursor = 0;
#ifdef ENABLE_DEBUGGER
        x = 0;
        y = 0;
#endif
    }

#ifdef ENABLE_DEBUGGER
    PixelRgb565* get_pixels() {
        return pixels;
    }
    uint16_t get_cursor() const {
        return cursor;
    }
#endif

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    Appearance appearance {};

    PixelRgb565 pixels[PIXEL_COUNT] {};
    uint16_t cursor {};

#ifdef ENABLE_DEBUGGER
    uint8_t x {};
    uint8_t y {};
#endif
};

#endif // LCD_H