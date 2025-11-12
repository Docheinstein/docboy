#ifndef WRAMBUS_H
#define WRAMBUS_H

#include "docboy/bus/bus.h"

class Wram1;
class Wram2;

class WramBus final : public Bus {

public:
    template <Device::Type Dev>
    using View = BusView<WramBus, Dev>;

    WramBus(Wram1& wram1, Wram2* wram2);

    void set_wram2_bank(uint8_t bank);

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    Wram1& wram1;
    Wram2* wram2;

    uint8_t wram2_bank {};

private:
    uint8_t read_wram2(uint16_t address) const;
    void write_wram2(uint16_t address, uint8_t value);
};
#endif // CPUBUS_H