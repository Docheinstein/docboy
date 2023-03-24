#ifndef CARTRIDGESLOT_H
#define CARTRIDGESLOT_H

#include <cstdint>
#include <memory>
#include "cartridge.h"

class ICartridgeSlot {
public:
    virtual ~ICartridgeSlot() = default;

    virtual void attachCartridge(std::unique_ptr<ICartridge> cartridge) = 0;
    virtual void detachCartridge() = 0;
};

class CartridgeSlot : public ICartridge, public ICartridgeSlot {
public:
    void attachCartridge(std::unique_ptr<ICartridge> cartridge) override;
    void detachCartridge() override;

    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;

    [[nodiscard]] Header header() const override;

private:
    std::unique_ptr<ICartridge> cartridge;
};


#endif // CARTRIDGESLOT_H