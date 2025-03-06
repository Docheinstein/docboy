#include "docboy/serial/serial.h"

#include "docboy/interrupts/interrupts.h"
#include "docboy/timers/timers.h"

#include "utils/asserts.h"
#include "utils/parcel.h"

Serial::Serial(Timers& timers, Interrupts& interrupts) :
    timers {timers},
    interrupts {interrupts} {
    reset();
}

void Serial::attach(ISerialEndpoint& e) {
    endpoint = &e;
}

void Serial::detach() {
    endpoint = nullptr;
}

bool Serial::is_attached() const {
    return endpoint != nullptr;
}

void Serial::tick() {
    ASSERT(progress < 8);

    // Serial transfer ticks each time DIV16[7] has a falling edge.
    // A bit is transferred after each two of these falling edges.
    const bool div_bit_7 = test_bit<7>(timers.div16);

    if (sc.transfer_enable && sc.clock_select) {
        if (prev_div_bit_7 && !div_bit_7) {
            // Falling edge: advance transfer.
            master_transfer_toggle = !master_transfer_toggle;

            if (!master_transfer_toggle) {
                // TODO: check the timing of the input_bit and output_bit with a connected endpoint:
                //       are they computed together in the second half of this bit transfer,
                //       or maybe one of them or both are computed in the first half?
                //       (Need second GameBoy to test it).
                bool input_bit;

                if (endpoint) {
                    // Read the bit from the other peripheral.
                    input_bit = endpoint->serial_read_bit();
                } else {
                    // Endpoint disconnected.
                    input_bit = true;
                }

                // Retrieve the next bit to sent from SB.
                const bool output_bit = serial_read_bit();

                // Update SB with the received bit.
                serial_write_bit(input_bit);

                if (endpoint) {
                    // Send bit to the other peripheral.
                    endpoint->serial_write_bit(output_bit);
                }
            }
        }
    }

    prev_div_bit_7 = div_bit_7;
}

bool Serial::serial_read_bit() {
    return test_bit<7>(sb);
}

void Serial::serial_write_bit(bool bit) {
    sb = sb << 1 | bit;

    if (++progress == 8) {
        // Transfer finished.
        sc.transfer_enable = false;

        // Raise serial interrupt.
        interrupts.raise_interrupt<Interrupts::InterruptType::Serial>();

        progress = 0;
    }
}

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
    // Writing to SC aborts the current transfer (at least for the master).

    // TODO: what happens if the slave writes to SC?
    //       is the progress of the transfer reset too?
    //       (Need second GameBoy to test it).
    if (sc.transfer_enable && sc.clock_select) {
        if (master_transfer_toggle) {
            // TODO: it seems that SB is shifted in this case: is the new bit always 1,
            //       or maybe it is the previous bit received from the other peripheral?
            //       or maybe it is retrieved instantly from the other peripheral?
            //       (Need second GameBoy to test it).
            constexpr bool input_bit = true;
            sb = sb << 1 | input_bit;
        }

        master_transfer_toggle = false;
        progress = 0;
    }

    sc.transfer_enable = test_bit<Specs::Bits::Serial::SC::TRANSFER_ENABLE>(value);
#ifdef ENABLE_CGB
    sc.clock_speed = test_bit<Specs::Bits::Serial::SC::CLOCK_SPEED>(value);
#endif
    sc.clock_select = test_bit<Specs::Bits::Serial::SC::CLOCK_SELECT>(value);
}

void Serial::save_state(Parcel& parcel) const {
    parcel.write_bool(prev_div_bit_7);
    parcel.write_uint8(progress);
    parcel.write_bool(master_transfer_toggle);

    parcel.write_uint8(sb);
    parcel.write_bool(sc.transfer_enable);
#ifdef ENABLE_CGB
    parcel.write_bool(sc.clock_speed);
#endif
    parcel.write_bool(sc.clock_select);
}

void Serial::load_state(Parcel& parcel) {
    prev_div_bit_7 = parcel.read_bool();
    progress = parcel.read_uint8();
    master_transfer_toggle = parcel.read_bool();

    sb = parcel.read_uint8();
    sc.transfer_enable = parcel.read_bool();
#ifdef ENABLE_CGB
    sc.clock_speed = parcel.read_bool();
#endif
    sc.clock_select = parcel.read_bool();
}

void Serial::reset() {
    prev_div_bit_7 = false;
    progress = 0;
    master_transfer_toggle = false;

    sb = 0;
    sc.transfer_enable = false;
#ifdef ENABLE_CGB
#ifdef ENABLE_BOOTROM
    sc.clock_speed = false;
#else
    sc.clock_speed = true;
#endif
#endif
#ifdef ENABLE_CGB
#ifdef ENABLE_BOOTROM
    sc.clock_select = false;
#else
    sc.clock_select = true;
#endif
#else
    sc.clock_select = false;
#endif
}