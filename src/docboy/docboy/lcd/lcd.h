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
    using Pixel = uint16_t /* [0:32767]*/;
#else
    using Pixel = uint8_t /* [0:3] */;
#endif

    struct Colors {
        static constexpr PixelRgb565 WHITE = 0xFFFF;
        static constexpr PixelRgb565 BLACK = 0x0000;
    };

#ifdef ENABLE_CGB
    static constexpr uint16_t NUM_COLORS = 32768;
#else
    static constexpr uint16_t NUM_COLORS = 4;
#endif

    static constexpr uint16_t PIXEL_COUNT = Specs::Display::WIDTH * Specs::Display::HEIGHT;
    static constexpr uint32_t PIXEL_BUFFER_SIZE = PIXEL_COUNT * sizeof(PixelRgb565);

    using Palette = std::array<uint16_t, NUM_COLORS>;

    Lcd();

    explicit Lcd(const Palette& palette) :
        palette {palette} {
    }

    void set_palette(const Palette& palette_) {
        palette = palette_;
    }

    void push_pixel(Pixel pixel) {
        ASSERT(pixel < NUM_COLORS);
        pixels[cursor] = palette[pixel];
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

    void reset_cursor() {
        // Reset the LCD position.
        cursor = 0;
#ifdef ENABLE_DEBUGGER
        x = 0;
        y = 0;
#endif
    }

    void clear(PixelRgb565 color) {
        // Fills the LCD with color 0.
        for (PixelRgb565& pixel : pixels) {
            pixel = color;
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
    Palette palette {};

    PixelRgb565 pixels[PIXEL_COUNT] {};
    uint16_t cursor {};

#ifdef ENABLE_DEBUGGER
    uint8_t x {};
    uint8_t y {};
#endif
};

#endif // LCD_H