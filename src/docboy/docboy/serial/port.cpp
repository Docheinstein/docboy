#include "docboy/serial/port.h"

#include "docboy/interrupts/interrupts.h"
#include "docboy/timers/timers.h"

#include "utils/asserts.h"

SerialPort::SerialPort(Timers& timers, Interrupts& interrupts) :
    timers {timers},
    interrupts {interrupts} {
}

void SerialPort::tick() {
    ASSERT(progress < 16);

    const bool is_transferring = sc.transfer_enable && sc.clock_select;

    if (transferring > is_transferring) {
        // Transfer is aborted.
        // If the serial transfer is in the second half of the current bit's transfer,
        // SB's content is updated anyhow with the input bit from the other peripheral.
        // TODO: actually check if the bit comes from the other peripheral or it's always 1.
        if (mod<2>(progress) == 1) {
            sb = sb << 1 | input_bit;
        }
        progress = 0;
    }

    transferring = is_transferring;

    // Serial transfer ticks each time DIV16[7] has a falling edge.
    // A bit is transferred after each two of these falling edges.
    const bool div_bit_7 = test_bit<7>(timers.div16);

    if (transferring) {
        if (prev_div_bit_7 && !div_bit_7) {
            // Falling edge: advance transfer.

            // TODO: check the timing of the single bit within two falling edges.
            // I assume that the bit is received after the first half of the bit's transfer
            // and not after the second half, that's because it seems that aborting the
            // transfer  after the first half updates SB with the received bit.
            if (mod<2>(progress) == 0) {
                if (endpoint) {
                    // Read the bit from the other peripheral.
                    input_bit = endpoint->serial_read_bit();
                } else {
                    // Endpoint disconnected
                    input_bit = true;
                }

                // TODO: is the output bit sent here, or in the second half?
                bool output_bit = test_bit<7>(sb);

                if (endpoint) {
                    endpoint->serial_write_bit(output_bit);
                }
            } else {
                // Update SB with the received bit.
                sb = sb << 1 | input_bit;
            }

            if (++progress == 16) {
                // Transfer finished.
                progress = 0;
                sc.transfer_enable = false;

                // Raise serial interrupt.
                interrupts.raise_interrupt<Interrupts::InterruptType::Serial>();
            }
        }
    }

    prev_div_bit_7 = div_bit_7;
}

void SerialPort::attach(ISerialEndpoint& e) {
    endpoint = &e;
}

void SerialPort::detach() {
    endpoint = nullptr;
}

void SerialPort::save_state(Parcel& parcel) const {
    Serial::save_state(parcel);
    parcel.write_bool(transferring);
    parcel.write_uint8(progress);
    parcel.write_bool(input_bit);
    parcel.write_bool(prev_div_bit_7);
}

void SerialPort::load_state(Parcel& parcel) {
    Serial::load_state(parcel);
    transferring = parcel.read_bool();
    progress = parcel.read_uint8();
    input_bit = parcel.read_bool();
    prev_div_bit_7 = parcel.read_bool();
}
