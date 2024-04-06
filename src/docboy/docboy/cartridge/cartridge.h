#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "docboy/shared/macros.h"
#include <cstdint>

class Parcel;

class ICartridge {
public:
    virtual ~ICartridge() = default;

    [[nodiscard]] virtual uint8_t readRom(uint16_t address) const = 0;
    virtual void writeRom(uint16_t address, uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readRam(uint16_t address) const = 0;
    virtual void writeRam(uint16_t address, uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t* getRamSaveData() = 0;
    [[nodiscard]] virtual uint32_t getRamSaveSize() const = 0;

    IF_DEBUGGER(virtual uint8_t* getRomData() = 0);
    IF_DEBUGGER(virtual uint32_t getRomSize() const = 0);

    virtual void saveState(Parcel& parcel) const = 0;
    virtual void loadState(Parcel& parcel) = 0;
};

#endif // CARTRIDGE_H