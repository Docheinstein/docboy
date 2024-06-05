#include "docboy/joypad/joypad.h"
#include "docboy/interrupts/interrupts.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/memsniffer.h"
#endif

uint8_t JoypadIO::read_p1() const {
    const uint8_t out =
        p1 | (test_bit<Specs::Bits::Joypad::P1::SELECT_DIRECTION_BUTTONS>(p1) ? keep_bits<4>(keys)
                                                                              : keep_bits<4>(keys >> 4));
#ifdef ENABLE_DEBUGGER
    DebuggerMemorySniffer::notify_memory_read(Specs::Registers::Joypad::P1, out);
#endif
    return out;
}

void JoypadIO::write_p1(uint8_t value) {
#ifdef ENABLE_DEBUGGER
    uint8_t old_value = p1;
#endif

    p1 = 0b11000000 | (value & 0b00110000);

#ifdef ENABLE_DEBUGGER
    DebuggerMemorySniffer::notify_memory_write(Specs::Registers::Joypad::P1, old_value, p1);
#endif
}

Joypad::Joypad(InterruptsIO& interrupts) :
    interrupts {interrupts} {
}

void Joypad::set_key_state(Key key, KeyState state) {
    set_bit(keys, static_cast<uint8_t>(key), static_cast<uint8_t>(state));

    const bool raise_interrupt =
        (state == KeyState::Pressed) &&
        (test_bit<Specs::Bits::Joypad::P1::SELECT_DIRECTION_BUTTONS>(p1) ? key <= Key::Start : key >= Key::Right);

    if (raise_interrupt) {
        interrupts.raise_Interrupt<InterruptsIO::InterruptType::Joypad>();
    }
}
