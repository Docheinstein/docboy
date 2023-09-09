#ifndef MBC3RAMBATTERY_H
#define MBC3RAMBATTERY_H

#include "core/cartridge/batteriedram.h"
#include "mbc3ram.h"

class MBC3RAMBattery : public MBC3RAM, public IBatteriedRAM {
public:
    explicit MBC3RAMBattery(const std::vector<uint8_t>& data);
    explicit MBC3RAMBattery(std::vector<uint8_t>&& data);

    // TODO: put these in a common class or use a component approach
    void loadRAM(IReadableSave& save) override;
    void saveRAM(IWritableSave& save) override;
};

#endif // MBC3RAMBATTERY_H