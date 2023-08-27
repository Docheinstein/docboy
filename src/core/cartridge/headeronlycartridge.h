#ifndef HEADERONLYCARTRIDGE_H
#define HEADERONLYCARTRIDGE_H

#include "cartridge.h"

class HeaderOnlyCartridge : public Cartridge {
public:
    explicit HeaderOnlyCartridge(const std::vector<uint8_t>& data);
    explicit HeaderOnlyCartridge(std::vector<uint8_t>&& data);

    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;
};

#endif // HEADERONLYCARTRIDGE_H