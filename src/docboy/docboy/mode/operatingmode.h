#ifndef OPERATINGMODE_H
#define OPERATINGMODE_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

class Parcel;
struct CartridgeHeader;

class OperatingMode {
public:
#ifdef ENABLE_BOOTROM
    OperatingMode();
#else
    explicit OperatingMode(CartridgeHeader& header);
#endif

    bool is_cgb_mode() const {
        return cgb_mode;
    }

    void write_key0(uint8_t value);
    uint8_t read_key0() const;

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

#ifndef ENABLE_BOOTROM
    CartridgeHeader& header;
#endif

    struct Key0 : Composite<Specs::Registers::OperatingMode::KEY0> {
        Bool dmg_mode {make_bool()};
        Bool dmg_ext_mode {make_bool()};
    } key0 {};

private:
    bool cgb_mode {};
};

#endif // OPERATINGMODE_H
