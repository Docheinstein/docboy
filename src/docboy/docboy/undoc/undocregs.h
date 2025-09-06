#ifndef UNDOCREGS_H
#define UNDOCREGS_H

#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

#include "utils/parcel.h"

class UndocumentedRegisters {
public:
    void write_ff75(uint8_t value);

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    UInt8 ff72 {make_uint8(Specs::Registers::Undocumented::FF72)};
    UInt8 ff73 {make_uint8(Specs::Registers::Undocumented::FF73)};
    UInt8 ff74 {make_uint8(Specs::Registers::Undocumented::FF74)};
    UInt8 ff75 {make_uint8(Specs::Registers::Undocumented::FF75)};
};

#endif // UNDOCREGS_H
