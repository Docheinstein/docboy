#ifndef CARTRIDGESLOT_H
#define CARTRIDGESLOT_H

#include "cartridge.h"
#include <cstdint>
#include <memory>

class ICartridgeSlot {
public:
    virtual ~ICartridgeSlot() = default;

    virtual void attachCartridge(std::unique_ptr<ICartridge> cartridge) = 0;
    virtual void detachCartridge() = 0;
    [[nodiscard]] virtual ICartridge* getAttachedCartridge() const = 0;
};

class CartridgeSlot : public ICartridge, public ICartridgeSlot {
public:
    void attachCartridge(std::unique_ptr<ICartridge> cartridge) override;
    void detachCartridge() override;
    [[nodiscard]] ICartridge* getAttachedCartridge() const override;

    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;

private:
    std::unique_ptr<ICartridge> cartridge;
};

#endif // CARTRIDGESLOT_H