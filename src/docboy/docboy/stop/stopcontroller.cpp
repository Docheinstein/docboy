#include "docboy/stop/stopcontroller.h"
#include "docboy/joypad/joypad.h"
#include "docboy/lcd/lcd.h"
#include "docboy/ppu/ppu.h"
#include "docboy/timers/timers.h"

#include "utils/bits.h"
#include "utils/parcel.h"

StopController::StopController(bool& stopped, Joypad& joypad, Timers& timers, Ppu& ppu, Lcd& lcd) :
    stopped {stopped},
    joypad {joypad},
    timers {timers},
    ppu {ppu},
    lcd {lcd} {
}

void StopController::stop() {
    ASSERT(!stopped);
    ASSERT(!requested);

    // Request STOP
    requested = true;
}

void StopController::enter_stop_mode() {
    ASSERT(!stopped);
    ASSERT(requested);

    // Enter STOP mode
    requested = false;
    stopped = true;

    // DIV is reset
    timers.div16 = 0;

    // Depending on the current PPU state, LCD behaves differently.
    //
    // DMG.
    //     PPU State   |    LCD
    // ----------------------------
    // Off             |  Off (neutral color)
    // OAM Scan        |  Off (neutral color)
    // Pixel Transfer  |  Off (neutral color)
    // HBlank          |  Off (neutral color)
    // VBlank          |  Off (neutral color)
    //
    // CGB.
    // - In case of real STOP:
    //     PPU State   |    LCD
    // ----------------------------
    // Off             |  Off (neutral color)
    // OAM Scan        |  Black
    // Pixel Transfer  |  Unchanged (keep the current content)
    // HBlank          |  Black
    // VBlank          |  Black

#ifdef ENABLE_CGB
    if (ppu.lcdc.enable) {
        if (ppu.stat.mode != Specs::Ppu::Modes::PIXEL_TRANSFER) {
            lcd.clear(Lcd::Colors::BLACK);
        }
    } else {
        lcd.clear(Lcd::Colors::WHITE);
    }
#else
    lcd.clear(Lcd::Colors::WHITE);
#endif
}

void StopController::eventually_exit_stop_mode() {
    ASSERT(stopped);
    ASSERT(!requested);

    // Exit STOP mode if there's joypad input
    stopped = keep_bits<4>(joypad.read_p1()) == bitmask<4>;
}

void StopController::save_state(Parcel& parcel) const {
    parcel.write_bool(stopped);
    parcel.write_bool(requested);
}

void StopController::load_state(Parcel& parcel) {
    stopped = parcel.read_bool();
    requested = parcel.read_bool();
}

void StopController::reset() {
    stopped = false;
    requested = false;
}
