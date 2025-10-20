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
    // In CGB, with high speed serial mode enabled, DIV16[2] is considered instead.
    const bool div_bit = timers.div16 & master.div_mask;

    if (sc.transfer_enable && sc.clock_select) {
        if (master.prev_div_bit && !div_bit) {
            // DIV bit had a falling edge.
            // After two falling edges, advance the transfer.
            master.toggle = !master.toggle;

            if (!master.toggle) {
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

    master.prev_div_bit = div_bit;
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
        if (master.toggle) {
            // TODO: it seems that SB updated in this case: is the new bit always 1,
            //       or maybe it is the previous bit received from the other peripheral?
            //       or maybe it is retrieved instantly from the other peripheral?
            //       (Need second GameBoy to test it).
            constexpr bool input_bit = true;
            sb = sb << 1 | input_bit;
        }

        progress = 0;
    }

    sc.transfer_enable = test_bit<Specs::Bits::Serial::SC::TRANSFER_ENABLE>(value);
#ifdef ENABLE_CGB
    sc.clock_speed = test_bit<Specs::Bits::Serial::SC::CLOCK_SPEED>(value);
#endif
    sc.clock_select = test_bit<Specs::Bits::Serial::SC::CLOCK_SELECT>(value);

#ifdef ENABLE_CGB
    master.div_mask = sc.clock_speed ? bit<2> : bit<7>;
#else
    master.div_mask = bit<7>;
#endif

    master.prev_div_bit = timers.div16 & master.div_mask;

    master.toggle = false;
}

void Serial::save_state(Parcel& parcel) const {
    PARCEL_WRITE_UINT8(parcel, sb);
    PARCEL_WRITE_BOOL(parcel, sc.transfer_enable);
#ifdef ENABLE_CGB
    PARCEL_WRITE_BOOL(parcel, sc.clock_speed);
#endif
    PARCEL_WRITE_BOOL(parcel, sc.clock_select);

    PARCEL_WRITE_UINT8(parcel, progress);
    PARCEL_WRITE_UINT16(parcel, master.div_mask);
    PARCEL_WRITE_BOOL(parcel, master.prev_div_bit);
    PARCEL_WRITE_BOOL(parcel, master.toggle);
}

void Serial::load_state(Parcel& parcel) {
    sb = parcel.read_uint8();
    sc.transfer_enable = parcel.read_bool();
#ifdef ENABLE_CGB
    sc.clock_speed = parcel.read_bool();
#endif
    sc.clock_select = parcel.read_bool();

    progress = parcel.read_uint8();
    master.div_mask = parcel.read_uint16();
    master.prev_div_bit = parcel.read_bool();
    master.toggle = parcel.read_bool();
}

void Serial::reset() {
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

    progress = 0;

#ifdef ENABLE_CGB
    master.div_mask = sc.clock_speed ? bit<2> : bit<7>;
#else
    master.div_mask = bit<7>;
#endif

    master.prev_div_bit = timers.div16 & master.div_mask;

    master.toggle = false;
}