#ifndef VRANBANKCONTROLLER_H
#define VRANBANKCONTROLLER_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/parcel.h"

class VramBus;

class VramBankController {
public:
    explicit VramBankController(VramBus& vram_bus) :
        vram_bus {vram_bus} {
    }

    uint8_t read_vbk() const;
    void write_vbk(uint8_t value);

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    struct Vbk : Composite<Specs::Registers::Banks::VBK> {
        UInt8 bank {make_bool()};
    } vbk {};

private:
    VramBus& vram_bus;
};

#endif // VRANBANKCONTROLLER_H