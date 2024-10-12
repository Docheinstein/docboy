#ifndef WRANBANKCONTROLLER_H
#define WRANBANKCONTROLLER_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/parcel.h"

class ExtBus;

class WramBankController {
public:
    explicit WramBankController(ExtBus& ext_bus) :
        ext_bus {ext_bus} {
    }

    uint8_t read_svbk() const;
    void write_svbk(uint8_t value);

    void save_state(Parcel& parcel) const {
        parcel.write_uint8(svbk.bank);
    }

    void load_state(Parcel& parcel) {
        svbk.bank = parcel.read_uint8();
    }

    void reset() {
        svbk.bank = 0;
    }

    struct Svbk : Composite<Specs::Registers::Banks::SVBK> {
        UInt8 bank {make_uint8()};
    } svbk {};

private:
    ExtBus& ext_bus;
};

#endif // VRANBANKCONTROLLER_H