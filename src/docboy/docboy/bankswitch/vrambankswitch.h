#ifndef VRAMBANKSWITCH_H
#define VRAMBANKSWITCH_H

#include "docboy/boot/boot.h"
#include "docboy/bus/vrambus.h"
#include "docboy/memory/cell.h"

class VramBankSwitch {
public:
    explicit VramBankSwitch(VramBus& vram_bus) :
        vram_bus {vram_bus} {
    }

    uint8_t read_vbk() const;
    void write_vbk(uint8_t value);

    void save_state(Parcel& parcel) const {
        parcel.write_bool(vbk.bank);
    }

    void load_state(Parcel& parcel) {
        vbk.bank = parcel.read_bool();
    }

    void reset() {
        vbk.bank = false;
    }

    struct Vbk : Composite<Specs::Registers::BankSwitch::VBK> {
        UInt8 bank {make_bool()};
    } vbk {};

private:
    VramBus& vram_bus;
};

#endif // VRAMBANKSWITCH_H
