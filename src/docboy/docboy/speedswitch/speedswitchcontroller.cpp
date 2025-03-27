#include "docboy/speedswitch/speedswitchcontroller.h"
#include "docboy/lcd/lcd.h"
#include "docboy/ppu/ppu.h"
#include "utils/parcel.h"

SpeedSwitchController::SpeedSwitchController(SpeedSwitch& speed_switch, Timers& timers, Ppu& ppu, Lcd& lcd,
                                             bool& halted) :
    speed_switch {speed_switch},
    timers {timers},
    ppu {ppu},
    lcd {lcd},
    halted {halted} {
}

void SpeedSwitchController::switch_speed() {
    halted = true;

    // TODO does it happen now, or when it finishes?
    speed_switch.key1.switch_speed = false;

    speed_switch.key1.current_speed = !speed_switch.key1.current_speed;

    speed_switch_countdown = 16386 * 2;
    speed_switch_timers_block_countdown = 4;

    if (!speed_switch.key1.current_speed) {
        speed_switch_countdown = 16386 * 4 - 5;
        speed_switch_timers_block_countdown = 2; // TODO: which is right? [2,3,4] seems equally valid
    }

    // Depending on the current PPU state, LCD behaves differently.
    //     PPU State   |    LCD
    // ----------------------------
    // Off             |  Off (neutral color)
    // OAM Scan        |  Unchanged (keep the current content)
    // Pixel Transfer  |  Unchanged (keep the current content)
    // HBlank          |  Unchanged (keep the current content)
    // VBlank          |  Unchanged (keep the current content)

    if (!ppu.lcdc.enable) {
        lcd.clear(Lcd::Colors::WHITE);
    }
}

void SpeedSwitchController::save_state(Parcel& parcel) const {
    parcel.write_uint32(speed_switch_countdown);
    parcel.write_uint32(speed_switch_timers_block_countdown);
}

void SpeedSwitchController::load_state(Parcel& parcel) {
    speed_switch_countdown = parcel.read_uint32();
    speed_switch_timers_block_countdown = parcel.read_uint32();
}

void SpeedSwitchController::reset() {
    speed_switch_countdown = 0;
    speed_switch_timers_block_countdown = 0;
}
