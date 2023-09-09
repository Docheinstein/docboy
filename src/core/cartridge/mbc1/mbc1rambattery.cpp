#include "mbc1rambattery.h"
#include "utils/arrayutils.h"
#include "utils/log.h"
#include <cstring>

MBC1RAMBattery::MBC1RAMBattery(const std::vector<uint8_t>& data) :
    MBC1RAM(data) {
}

MBC1RAMBattery::MBC1RAMBattery(std::vector<uint8_t>&& data) :
    MBC1RAM(data) {
}

void MBC1RAMBattery::loadRAM(IReadableSave& save) {
    const auto& data = save.readData();
    if (array_size(ram) != data.size()) {
        WARN() << "Loaded save is " << data.size() << " bytes but cartridge's RAM is " << array_size(ram) << " bytes"
               << std::endl;
    }
    const auto safeSize = std::min(array_size(ram), data.size());
    memcpy(ram, data.data(), safeSize);
}

void MBC1RAMBattery::saveRAM(IWritableSave& save) {
    save.writeData(ram, array_size(ram));
}
