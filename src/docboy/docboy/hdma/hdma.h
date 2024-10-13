#ifndef HDMA_H
#define HDMA_H

#include <cstdint>

#include "docboy/bus/oambus.h"
#include "docboy/common/macros.h"
#include "docboy/mmu/mmu.h"

class OamBus;
class Parcel;

class Hdma {
    DEBUGGABLE_CLASS()

public:
    explicit Hdma();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    void write_hdma1(uint8_t value);

    void write_hdma2(uint8_t value);

    void write_hdma3(uint8_t value);

    void write_hdma4(uint8_t value);

    uint8_t read_hdma5() const;
    void write_hdma5(uint8_t value);
};

#endif // HDMA_H
