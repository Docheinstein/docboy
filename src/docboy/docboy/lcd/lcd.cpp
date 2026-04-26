#include "docboy/lcd/lcd.h"

#include <cstring>

#include "docboy/lcd/colormap.h"

#include "utils/parcel.h"

// Some notes/thoughts about the LCD.
//
// Aside from the obvious differences regarding the color, DMG and CGB screens (LCD) are
// also very different regarding the underlying used technology.
//
// For both DMG and CGB, the LCD does not render pixels one-by-one, as it would require
// a lot of electric wiring to allow this kind of precise addressing.
//
// Instead, the LCD copies one row at a time. This means that PPU pushes to a temporary buffer
// (not directly to screen) and LCD/PPU are synchronized so that LCD copies the buffer
// to the current selected display row (approximately at the end of the line).
//
// Evidence shows that LCD and PPU are not hard-synchronized, and by that I mean that both are
// ticked by the clock and usually run in sync (e.g. LCD copies the buffer at the end of HBlank),
// but when PPU is turned off that synchronization is lost and things get more crazy.
// Indeed, LCD continues to tick, and it clears the further lines (even if PPU effectively does not tick).
// When PPU is turned on again, the LCD "waits" PPU to reach a new frame, and when they met they start again to
// render the new frame in sync (I've simplified some details, see the implementation).
//
// Another major difference is that DMG uses a Passive Matrix Addressing LCD: this kind of display does not have
// any mechanism to keep the state of the pixel, indeed the color of the pixel decays pretty fast (not measured).
// On the other hand, CGB uses an Active Matrix Addressing LCD: this screen basically have a capacitor for each
// pixel that retain the state of the pixel until it is overwritten.
//
// This difference between the two types of LCDs has some visible effects. For example, when PPU is turned off
// in the middle of the frame, in CGB the rows already rendered remain rendered until the next frame,
// when they will finally be cleared.
// Whereas doing the same in DMG shows that the pixels already drawn decay rapidly: the effect is that the screen
// seems to be "instantly" cleared.
//
// More tricky tests have shown that, on CGB, LCD can also be "stalled". For example, if PPU is turned off in
// the middle of a frame and then is rapidly turned on and off, the already drawn lines just sit there "forever"
// (even if PPU never write them again) and LCD never reaches the "synchronization" point, so effectively nothing
// new is rendered.
//
// Note: most of the logic here below have been tested on CGB, as it has been possible and more
// practical to record it with a slow-motion camera.
// Doing that on DMG is trickier, as the absence of capacitors makes the slow-motion videos "blinky",
// so the logic in DMG could possibly be different. For the sake of simplicity, I kept the same logic for both,
// as I don't have a clear proof telling me that the internal logic of DMG is different, neither.

Lcd::Lcd() {
    appearance = DEFAULT_APPEARANCE;
    reset();
}

void Lcd::tick() {
    if (wait_new_frame) {
        // LCD already reached internal synchronization point
        // (row 148) and is waiting for PPU to enter a new frame.
        // Nothing to do.
        return;
    }

    ASSERT(row <= Specs::Display::HEIGHT);

    if (row < Specs::Display::HEIGHT) {
        ++row_ticks;

        if (row_ticks == 256) {
            // If PPU is turned off the internal LCD row selector will usually be increased.
            // Surprisingly, this does not seem to happen if LCD is turned off rapidly
            // since the last LCD reset (at least 256 ticks should pass).
            // This flag keeps track of the fact the LCD reached that point.
            row_increment_enabled = true;
        } else if (row_ticks == 452) {
            if (rendering_enabled) {
                // Standard case.
                // LCD is synchronized with PPU, so LCD can copy all the pixels PPU pushed for the current row.
                ASSERT(row_cursor == Specs::Display::WIDTH);
                render_row();
            } else {
                // Clearing case.
                // PPU has been turned off so LCD will clear the line instead of render it.
                clear_row();
            }
        } else if (row_ticks == 456) {
            // Advance LCD row selector.
            ++row;

            row_ticks = 0;

#ifdef ENABLE_DIRECT_LCD_RENDERING
            row_cursor = 0; // Just for having a continuous pixel-cursor indicator.
#endif
        }
    }

    if (++frame_ticks == 67488 /* 148 row * 114 cycles */) {
        // LCD reaches a synchronization point after 148 rows from the last activation.
        // Depending on the PPU state, there are two possibilities:
        // 1) If PPU is on, LCD is stalled until PPU actually starts a new frame, so that the two can
        //    run in sync for the new frame.
        // 2) If PPU is off, LCD proceed to clear the next frame.
        // This behavior has some practical implications: for example, if PPU is turned off during VBlank
        // (as it should be), and turned on BEFORE this synchronization point (i.e. before LCD row 148),
        // the LCD will correctly render the next frame and no line/frame/blink will happen.
        // On the other hand, even if PPU is turned off during VBlank but is turned off after this synchronization point
        // (e.g. LCD row 150), the LCD already began to clear the next frame and so one cleared/blinking frame will be
        // shown.
        wait_new_frame = ppu_on;

        row = 0;
    }
}

void Lcd::turn_on_ppu() {
    ppu_on = true;

    // The frame counter is reset when PPU is turned on.
    // It will count up to 148 rows from now.
    frame_ticks = 0;
}

void Lcd::turn_off_ppu() {
    ASSERT(row <= Specs::Display::HEIGHT);

    if (row < Specs::Display::HEIGHT) {
        // In CGB, if LCD is turned off in the middle of a frame,
        // the current row's pixels are pushed top the display.
        render_row();

        // The internal row selector advances as well, but
        // this seems to happen only if at least 64 cycles has
        // passed since the last time LCD was turned off.
        // This means that if LCD is "rapidly" turned on and off,
        // there's a change to actually stall the LCD's internal
        // row selector and prevent it to render the further lines.
        if (row_increment_enabled) {
            row++;
        }
    }

    ppu_on = false;

    // LCD won't render PPU pixels until the next frame.
    rendering_enabled = false;

    row_increment_enabled = false;

    // Eventually abort any waiting for the new frame.
    wait_new_frame = false;

    // The row ticks counter is reset when PPU is turned off.
    // Note that it will tick even while PPU is off (as LCD will be cleared even with PPU off).
    row_ticks = 0;

#ifndef ENABLE_CGB
    // On DMG pixels color decay rapidly if they are not constantly refreshed by the PPU/LCD.
    // They probably are not cleared instantly, but since I wouldn't emulate the physical decay time
    // of each pixel's liquid crystal here I clear the entire screen atomically.
    // Note: screen is not cleared on CGB as the pixels are kept active by internal switch/capacitors.
    clear();
#endif
}

void Lcd::new_frame() {
    // If PPU enters a new frame and LCD is ready for it (it reached the internal row 148),
    // PPU is effectively synchronized with LCD again and LCD can start to render PPU pixels.
    // Otherwise, LCD will continue until it reaches internal row 148 and the next PPU frame
    // will be discarded (LCD will be cleared for a frame).
    if (wait_new_frame) {
        ASSERT(row_ticks == 0);

        rendering_enabled = true;
        wait_new_frame = false;

        frame_ticks = 0;
    }
}

void Lcd::set_appearance(const Appearance& a) {
    appearance = a;
}

void Lcd::save_state(Parcel& parcel) const {
    PARCEL_WRITE_BYTES(parcel, pixels, sizeof(pixels));
#ifndef ENABLE_DIRECT_LCD_RENDERING
    PARCEL_WRITE_BYTES(parcel, row_pixels, sizeof(row_pixels));
#endif
    PARCEL_WRITE_UINT8(parcel, row);
    PARCEL_WRITE_UINT8(parcel, row_cursor);
    PARCEL_WRITE_UINT16(parcel, row_ticks);
    PARCEL_WRITE_UINT32(parcel, frame_ticks);
    PARCEL_WRITE_BOOL(parcel, ppu_on);
    PARCEL_WRITE_BOOL(parcel, rendering_enabled);
    PARCEL_WRITE_BOOL(parcel, row_increment_enabled);
    PARCEL_WRITE_BOOL(parcel, wait_new_frame);
}

void Lcd::load_state(Parcel& parcel) {
    parcel.read_bytes(pixels, sizeof(pixels));
#ifndef ENABLE_DIRECT_LCD_RENDERING
    parcel.read_bytes(row_pixels, sizeof(row_pixels));
#endif
    row = parcel.read_uint8();
    row_cursor = parcel.read_uint8();
    row_ticks = parcel.read_uint16();
    frame_ticks = parcel.read_uint32();
    ppu_on = parcel.read_bool();
    rendering_enabled = parcel.read_bool();
    row_increment_enabled = parcel.read_bool();
    wait_new_frame = parcel.read_bool();
}

void Lcd::reset() {
    memset(pixels, 0, sizeof(pixels));
#ifndef ENABLE_DIRECT_LCD_RENDERING
    memset(row_pixels, 0, sizeof(row_pixels));
#endif

    row = 0;
    row_cursor = 0;

    row_ticks = 0;
    frame_ticks = 67488;

    ppu_on = true;

    row_increment_enabled = false;
    rendering_enabled = true;

    wait_new_frame = true;
}

void Lcd::render_row() {
#ifndef ENABLE_DIRECT_LCD_RENDERING
    ASSERT(row < Specs::Display::HEIGHT);

    // Effectively copy the row pixels to the display buffer.
    uint16_t* lcd_row = pixels + Specs::Display::WIDTH * row;

    for (uint16_t i = 0; i < Specs::Display::WIDTH; ++i) {
        lcd_row[i] = row_pixels[i];

        // The internal row pixels seems to be cleared while doing so:
        // this is shown by some tests with PPU turned off in the
        // middle of a frame (under standard condition it wouldn't even
        // be necessary, as PPU will overwrite such pixels the next line).
        row_pixels[i] = appearance.default_color;
    }
#endif
}

void Lcd::clear_row() {
    ASSERT(row < Specs::Display::HEIGHT);

    // In clearing mode, the current display row is cleared with the LCD default color.
    uint16_t* lcd_row = pixels + Specs::Display::WIDTH * row;

    for (uint16_t i = 0; i < Specs::Display::WIDTH; ++i) {
        lcd_row[i] = appearance.default_color;

#ifndef ENABLE_DIRECT_LCD_RENDERING
        // The internal row pixels seem to be cleared while doing so:
        // this is shown by some tests where PPU is turned off in the
        // middle of a frame (under standard conditions it wouldn't even
        // be necessary, as PPU would overwrite such pixels the next line).
        row_pixels[i] = appearance.default_color;
#endif
    }
}