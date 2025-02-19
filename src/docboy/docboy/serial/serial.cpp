#include "docboy/serial/serial.h"

#include "utils/bits.h"

uint8_t Serial::read_sc() const {
#ifdef ENABLE_CGB
    return 0b01111100 | sc.transfer_enable << Specs::Bits::Serial::SC::TRANSFER_ENABLE |
           sc.clock_speed << Specs::Bits::Serial::SC::CLOCK_SPEED |
           sc.clock_select << Specs::Bits::Serial::SC::CLOCK_SELECT;
#else
    return 0b01111110 | sc.transfer_enable << Specs::Bits::Serial::SC::TRANSFER_ENABLE |
           sc.clock_select << Specs::Bits::Serial::SC::CLOCK_SELECT;
#endif
}

void Serial::write_sc(uint8_t value) {
    sc.transfer_enable = test_bit<Specs::Bits::Serial::SC::TRANSFER_ENABLE>(value);
#ifdef ENABLE_CGB
    sc.clock_speed = test_bit<Specs::Bits::Serial::SC::CLOCK_SPEED>(value);
#endif
    sc.clock_select = test_bit<Specs::Bits::Serial::SC::CLOCK_SELECT>(value);
}

void Serial::save_state(Parcel& parcel) const {
    parcel.write_uint8(sb);
    parcel.write_bool(sc.transfer_enable);
#ifdef ENABLE_CGB
    parcel.write_bool(sc.clock_select);
#endif
    parcel.write_bool(sc.clock_select);
}

void Serial::load_state(Parcel& parcel) {
    sb = parcel.read_uint8();
    sc.transfer_enable = parcel.read_bool();
#ifdef ENABLE_CGB
    sc.clock_speed = parcel.read_bool();
#endif
    sc.clock_select = parcel.read_bool();
}

void Serial::reset() {
    sb = 0;
    sc.transfer_enable = false;
#ifdef ENABLE_CGB
    sc.clock_speed = false;
#endif
    sc.clock_select = false;
}
