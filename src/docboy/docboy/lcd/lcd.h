#ifndef LCD_H
#define LCD_H

#include <array>

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

    void tick();

    void turn_on_ppu();
    void turn_off_ppu();

    void new_frame();

    void set_appearance(const Appearance& a);

    void push_pixel(Pixel pixel) {
        ASSERT(pixel < Appearance::NUM_COLORS);
        ASSERT(row_cursor < Specs::Display::WIDTH);
#ifdef ENABLE_DIRECT_LCD_RENDERING
        // Although this rendering mode could be more efficient, it is meant to be used for debug purpose mainly,
        // as Game Boy does not render pixels directly. Instead, the PPU pushes the pixels a temporary buffer
        // and then the entire row is copied to the real LCD framebuffer atomically.
        // Therefore, do not expect the emulator to pass all the tricky test roms in this mode.
        if (row < Specs::Display::HEIGHT) {
            pixels[row * Specs::Display::WIDTH + row_cursor++] = appearance.palette[pixel];
        }
#else
        row_pixels[row_cursor++] = appearance.palette[pixel];
#endif
    }

    const PixelRgb565* get_pixels() const {
        return pixels;
    }

    void reset_row_cursor() {
        row_cursor = 0;
    }

#ifndef ENABLE_CGB
    void clear() {
        // Fills all the LCD pixels with the default color.
        for (uint32_t i = 0; i < array_size(pixels); ++i) {
            pixels[i] = appearance.default_color;
        }
    }
#endif

#ifdef ENABLE_DEBUGGER
    PixelRgb565* get_pixels() {
        return pixels;
    }

    uint16_t get_cursor() const {
#ifdef ENABLE_DIRECT_LCD_RENDERING
        return row * Specs::Display::WIDTH + row_cursor;
#else
        return row * Specs::Display::WIDTH;
#endif
    }
#endif

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    void render_row();
    void clear_row();

    Appearance appearance {};

    PixelRgb565 pixels[PIXEL_COUNT] {};
#ifndef ENABLE_DIRECT_LCD_RENDERING
    PixelRgb565 row_pixels[Specs::Display::WIDTH] {};
#endif

    uint8_t row {};
    uint8_t row_cursor {};

    uint16_t row_ticks {};
    uint32_t frame_ticks {};

    bool ppu_on {};

    bool rendering_enabled {};
    bool row_increment_enabled {};
    bool wait_new_frame {};
};

#endif // LCD_H