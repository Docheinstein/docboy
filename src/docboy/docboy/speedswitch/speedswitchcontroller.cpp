#include "docboy/speedswitch/speedswitchcontroller.h"

#include "docboy/interrupts/interrupts.h"
#include "docboy/timers/timers.h"

#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/parcel.h"

SpeedSwitchController::SpeedSwitchController(SpeedSwitch& speed_switch, Interrupts& interrupts, Timers& timers,
                                             bool& halted) :
    speed_switch {speed_switch},
    interrupts {interrupts},
    timers {timers},
    halted {halted} {
}

void SpeedSwitchController::switch_speed() {
    // Halt CPU.
    halted = true;

    speed_switch.key1.switch_speed = false;

    if (!speed_switch.key1.current_speed) {
        // Single => Double

        // Speed switch actually takes effect after 4 T-Cycles.
        speed_switch_enter_countdown = 2;

        speed_switch_exit_countdown = 16386 * 2;

        // It seems that interrupts are blocked for the next 6 T-Cycles.
        interrupts_block.blocked = true;
        interrupts_block.countdown = 3;

        interrupts_double_speed = true;

        timers_block.blocked = true;
        timers_block.countdown = 4;

        // DMA will be blocked after 6 T-Cycles.
        ASSERT(!dma_block.blocked);
        dma_block.countdown = 3;
    } else {
        // Double => Single

        // Speed switch actually takes effect after 4 T-Cycles.
        speed_switch_enter_countdown = 2;

        speed_switch_exit_countdown = 16384 * 4 + 2;

        interrupts_double_speed = false;

        // Timer is blocked for the next 4 T-Cycles.
        timers_block.blocked = true;
        timers_block.countdown = 2;

        // DMA will be blocked the next T-Cycle.
        ASSERT(!dma_block.blocked);
        dma_block.countdown = 1;

        // Switch from double to single seems to block the PPU for one T-Cycle.
        ppu_block.blocked = true;
    }
}

void SpeedSwitchController::save_state(Parcel& parcel) const {
    parcel.write_uint32(speed_switch_exit_countdown);
    parcel.write_uint8(speed_switch_enter_countdown);
    parcel.write_bool(timers_block.blocked);
    parcel.write_uint8(timers_block.countdown);
    parcel.write_bool(interrupts_block.blocked);
    parcel.write_uint8(interrupts_block.countdown);
    parcel.write_bool(dma_block.blocked);
    parcel.write_uint8(dma_block.countdown);
    parcel.write_bool(ppu_block.blocked);
    parcel.write_bool(interrupts_double_speed);
}

void SpeedSwitchController::load_state(Parcel& parcel) {
    speed_switch_exit_countdown = parcel.read_uint32();
    speed_switch_enter_countdown = parcel.read_uint8();
    timers_block.blocked = parcel.read_bool();
    timers_block.countdown = parcel.read_uint8();
    interrupts_block.blocked = parcel.read_bool();
    interrupts_block.countdown = parcel.read_uint8();
    dma_block.blocked = parcel.read_bool();
    dma_block.countdown = parcel.read_uint8();
    ppu_block.blocked = parcel.read_bool();
    interrupts_double_speed = parcel.read_bool();
}

void SpeedSwitchController::reset() {
    speed_switch_exit_countdown = 0;
    speed_switch_enter_countdown = 0;
    timers_block.blocked = false;
    timers_block.countdown = 0;
    interrupts_block.blocked = false;
    interrupts_block.countdown = 0;
    dma_block.blocked = false;
    dma_block.countdown = 0;
    ppu_block.blocked = false;
    interrupts_double_speed = false;
}

void SpeedSwitchController::tick() {
    if (speed_switch_enter_countdown) {
        if (--speed_switch_enter_countdown == 0) {
            // Actually change speed.
            speed_switch.key1.current_speed = !speed_switch.key1.current_speed;
        }
    }

    if (speed_switch_exit_countdown) {
        if (halted) {
            if (--speed_switch_exit_countdown == 0) {
                // Speed switch countdown finished.

                // Unhalt CPU.
                halted = false;

                // It seems that DMA unblock is delayed by 1 M-Cycle in this case.
                ASSERT(dma_block.blocked);
                dma_block.countdown = 2;
            }
        } else {
            // Speed switch has been exited prematurely (due to a pending interrupt).
            speed_switch_exit_countdown = 0;

            // DMA is unblocked instantly in this case.
            dma_block.blocked = false;
            dma_block.countdown = 0;
        }
    }

    if (dma_block.countdown) {
        if (--dma_block.countdown == 0) {
            // Either block or unblock DMA.
            dma_block.blocked = !dma_block.blocked;
        }
    }

    if (interrupts_block.countdown) {
        --interrupts_block.countdown;

        if (interrupts_block.countdown == 1) {
            // Usually interrupts are blocked for 6 T-Cycles, but if an interrupt was
            // pending, or it is raised in the first 4 T-Cycles, interrupt are unblocked
            // 2 T-Cycles earlier than usual.
            if (keep_bits<5>(interrupts.IE & interrupts.IF)) {
                interrupts_block.countdown = 0;
            }
        }

        if (interrupts_block.countdown == 0) {
            interrupts_block.blocked = false;
        }
    }

    if (timers_block.countdown) {
        if (--timers_block.countdown == 0) {

            // Unblock timers.
            timers_block.blocked = false;

            // DIV is reset (indeed DIV's falling edge may increase TIMA).
            // Quirk: it seems that there is a window of an additional DIV tick for
            // increase TIMA on falling edge when TIMA runs at 262kHz.
            // What really happens inside is still obscure to me.
            // Keeping the last four bits of DIV and allowing it to tick before
            // it's reset has been the most reasonable way to pass the test roms
            // that came to my mind.
            timers.set_div(keep_bits<4>(timers.div16));
            timers.tick();
            timers.set_div(0);
        }
    }

    ppu_block.blocked = false;
}
