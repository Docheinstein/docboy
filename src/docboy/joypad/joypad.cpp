#include "joypad.h"
#include "docboy/debugger/macros.h"
#include "docboy/interrupts/interrupts.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/memsniffer.h"
#endif

uint8_t JoypadIO::readP1() const {
    const uint8_t out =
        P1 | (test_bit<Specs::Bits::Joypad::P1::SELECT_DIRECTION_BUTTONS>(P1) ? keep_bits<4>(keys)
                                                                              : keep_bits<4>(keys >> 4));
    IF_DEBUGGER(DebuggerMemorySniffer::notifyMemoryRead(Specs::Registers::Joypad::P1, out));
    return out;
}

void JoypadIO::writeP1(uint8_t value) {
    IF_DEBUGGER(uint8_t oldValue = P1);
    P1 = 0b11000000 | (value & 0b00110000);
    IF_DEBUGGER(DebuggerMemorySniffer::notifyMemoryWrite(Specs::Registers::Joypad::P1, oldValue, P1));
}

Joypad::Joypad(InterruptsIO& interrupts) :
    interrupts(interrupts) {
}

void Joypad::setKeyState(Joypad::Key key, Joypad::KeyState state) {
    set_bit(keys, static_cast<uint8_t>(key), static_cast<uint8_t>(state));

    const bool raiseInterrupt =
        (state == KeyState::Pressed) &&
        (test_bit<Specs::Bits::Joypad::P1::SELECT_DIRECTION_BUTTONS>(P1) ? key <= Key::Start : key >= Key::Right);

    if (raiseInterrupt) {
        interrupts.raiseInterrupt<InterruptsIO::InterruptType::Joypad>();
    }
}
