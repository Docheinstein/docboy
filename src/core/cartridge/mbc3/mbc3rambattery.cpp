#include "mbc3rambattery.h"
#include "utils/arrayutils.h"
#include "utils/log.h"
#include <cstring>

MBC3RAMBattery::MBC3RAMBattery(const std::vector<uint8_t>& data) :
    MBC3RAM(data) {
}

MBC3RAMBattery::MBC3RAMBattery(std::vector<uint8_t>&& data) :
    MBC3RAM(data) {
}

void MBC3RAMBattery::loadRAM(IReadableSave& save) {
    const auto& data = save.readData();
    if (array_size(ram) != data.size()) {
        WARN() << "Loaded save is " << data.size() << " bytes but cartridge's RAM is " << array_size(ram) << " bytes"
               << std::endl;
    }
    const auto safeSize = std::min(array_size(ram), data.size());
    memcpy(ram, data.data(), safeSize);
}

void MBC3RAMBattery::saveRAM(IWritableSave& save) {
    save.writeData(ram, array_size(ram));
}
