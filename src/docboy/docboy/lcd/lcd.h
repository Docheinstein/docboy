#ifndef LCD_H
#define LCD_H

#include <array>
#include <cstring>

#include "docboy/common/macros.h"
#include "docboy/common/specs.h"

#include "utils/asserts.h"
#include "utils/bits.h"

class Parcel;

class Lcd {
    DEBUGGABLE_CLASS()

public:
    using PixelRgb565 = uint16_t;

#ifdef ENABLE_CGB
    using Pixel = uint16_t;
#else
    using Pixel = uint8_t;
#endif

    static constexpr uint16_t PIXEL_COUNT = Specs::Display::WIDTH * Specs::Display::HEIGHT;
    static constexpr uint32_t PIXEL_BUFFER_SIZE = PIXEL_COUNT * sizeof(PixelRgb565);

#ifndef ENABLE_CGB
    using Palette = std::array<uint16_t, 4>;

    static constexpr Palette DEFAULT_PALETTE {
        0x84A0, // Lightest Green
        0x4B40, // Light Green
        0x2AA0, // Dark Green
        0x1200, // Darkest Green
    };
#endif

#ifndef ENABLE_CGB
    explicit Lcd(const Palette& palette = DEFAULT_PALETTE) :
        palette {palette} {
    }

    void set_palette(const Palette& palette_) {
        palette = palette_;
    }
#endif

#ifdef ENABLE_CGB
    void push_pixel(Pixel pixel) {
        // TODO: rgb555 to rgb565
        uint8_t r5 = get_bits_range<14, 10>(pixel);
        uint8_t g5 = get_bits_range<9, 5>(pixel);
        uint8_t b5 = get_bits_range<4, 0>(pixel);

        uint8_t g6 = (g5 << 1);

        PixelRgb565 rgb565 = r5 << 11 | g6 << 5 | b5;
        //        PixelRgb565 rgb565 = b5 << 11 | g6 << 5  | r5;

        pixels[cursor] = rgb565;
        //        pixels[cursor] = pixel;
        if (++cursor == PIXEL_COUNT) {
            cursor = 0;
        }
#ifdef ENABLE_DEBUGGER
        x = (x + 1) % Specs::Display::WIDTH;
        y = (y + (x == 0)) % Specs::Display::HEIGHT;
#endif
    }

#else
    void push_pixel(Pixel pixel) {
        ASSERT(static_cast<uint8_t>(pixel) < 4);
        pixels[cursor] = palette[static_cast<uint8_t>(pixel)];
        if (++cursor == PIXEL_COUNT) {
            cursor = 0;
        }
#ifdef ENABLE_DEBUGGER
        x = (x + 1) % Specs::Display::WIDTH;
        y = (y + (x == 0)) % Specs::Display::HEIGHT;
#endif
    }
#endif

    const PixelRgb565* get_pixels() const {
        return pixels;
    }

    void reset_cursor() {
        // Reset the LCD position.
        cursor = 0;
#ifdef ENABLE_DEBUGGER
        x = 0;
        y = 0;
#endif
    }

    void clear() {
        // Fills the LCD with color 0.
        for (unsigned short& pixel : pixels) {
            pixel = 0xFFFF;
        }
    }

#ifdef ENABLE_DEBUGGER
    Lcd::PixelRgb565* get_pixels() {
        return pixels;
    }
    uint16_t get_cursor() const {
        return cursor;
    }
#endif

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset() {
        memset(pixels, 0, sizeof(pixels));
        cursor = 0;
#ifdef ENABLE_DEBUGGER
        x = 0;
        y = 0;
#endif
    }

private:
#ifndef ENABLE_CGB
    Palette palette {};
#endif

    PixelRgb565 pixels[PIXEL_COUNT] {};
    uint16_t cursor {};

#ifdef ENABLE_DEBUGGER
    uint8_t x {};
    uint8_t y {};
#endif
};

#endif // LCD_H