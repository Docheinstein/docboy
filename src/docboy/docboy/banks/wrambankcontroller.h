#ifndef WRANBANKCONTROLLER_H
#define WRANBANKCONTROLLER_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/parcel.h"

class WramBus;

class WramBankController {
public:
    explicit WramBankController(WramBus& wram_bus) :
        wram_bus {wram_bus} {
    }

    uint8_t read_svbk() const;
    void write_svbk(uint8_t value);

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    struct Svbk : Composite<Specs::Registers::Banks::SVBK> {
        UInt8 bank {make_uint8()};
    } svbk {};

private:
    WramBus& wram_bus;
};

#endif // VRANBANKCONTROLLER_H