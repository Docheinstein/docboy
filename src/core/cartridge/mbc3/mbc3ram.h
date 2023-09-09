#ifndef MBC3RAM_H
#define MBC3RAM_H

#include "mbc3.h"

class MBC3RAM : public MBC3 {
public:
    explicit MBC3RAM(const std::vector<uint8_t>& data);
    explicit MBC3RAM(std::vector<uint8_t>&& data);

    void loadState(IReadableState& state) override;
    void saveState(IWritableState& state) override;

    [[nodiscard]] uint8_t read(uint16_t address) const override;
    void write(uint16_t address, uint8_t value) override;

protected:
    uint8_t ram[0x8000];
};

#endif // MBC3RAM_H