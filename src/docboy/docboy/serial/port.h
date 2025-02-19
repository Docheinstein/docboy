#ifndef SERIALPORT_H
#define SERIALPORT_H

#include "docboy/serial/endpoint.h"
#include "docboy/serial/serial.h"

#include "docboy/common/macros.h"

class Timers;
class Interrupts;

class SerialPort : public Serial {
    DEBUGGABLE_CLASS()

public:
    explicit SerialPort(Timers& timers, Interrupts& interrupts);

    void attach(ISerialEndpoint& endpoint);
    void detach();

    void tick();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

private:
    Timers& timers;
    Interrupts& interrupts;

    ISerialEndpoint* endpoint;

    bool transferring {};
    uint8_t progress {}; // [0, 15]

    bool input_bit;

    bool prev_div_bit_7 {};
};

#endif // SERIALPORT_H