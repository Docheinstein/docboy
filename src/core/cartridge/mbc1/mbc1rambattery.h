#ifndef MBC1RAMBATTERY_H
#define MBC1RAMBATTERY_H

#include "core/cartridge/batteriedram.h"
#include "mbc1ram.h"

class MBC1RAMBattery : public MBC1RAM, public IBatteriedRAM {
public:
    explicit MBC1RAMBattery(const std::vector<uint8_t>& data);
    explicit MBC1RAMBattery(std::vector<uint8_t>&& data);

    // TODO: put these in a common class or use a component approach
    void loadRAM(IReadableSave& save) override;
    void saveRAM(IWritableSave& save) override;
};

#endif // MBC1RAMBATTERY_H