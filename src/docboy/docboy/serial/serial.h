#ifndef SERIAL_H
#define SERIAL_H

#include "docboy/common/macros.h"
#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"
#include "docboy/serial/endpoint.h"
#include "docboy/serial/serial.h"

class Parcel;
class Timers;
class Interrupts;

class Serial : public ISerialEndpoint {
    DEBUGGABLE_CLASS()

public:
    explicit Serial(Timers& timers, Interrupts& interrupts);

    void tick();

    uint8_t read_sc() const;
    void write_sc(uint8_t value);

    void attach(ISerialEndpoint& endpoint);
    void detach();

    bool is_attached() const;

    bool serial_read_bit() override;
    void serial_write_bit(bool bit) override;

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    UInt8 sb {make_uint8(Specs::Registers::Serial::SB)};

    struct Sc : Composite<Specs::Registers::Serial::SC> {
        Bool transfer_enable {make_bool()};
#ifdef ENABLE_CGB
        Bool clock_speed {make_bool()};
#endif
        Bool clock_select {make_bool()};
    } sc {};

private:
    Timers& timers;
    Interrupts& interrupts;

    ISerialEndpoint* endpoint {};

    uint8_t progress {};

    struct {
        uint16_t div_mask {};
        bool prev_div_bit {};
        bool toggle {};
    } master;
};

#endif // SERIAL_H