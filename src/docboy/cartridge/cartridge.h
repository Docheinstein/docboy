#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>

static constexpr uint32_t ROM_BANK_SIZE = 16 << 10;
static constexpr uint32_t RAM_BANK_SIZE = 8 << 10;

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

    virtual void saveState(Parcel& parcel) const = 0;
    virtual void loadState(Parcel& parcel) = 0;
};

#endif // CARTRIDGE_H