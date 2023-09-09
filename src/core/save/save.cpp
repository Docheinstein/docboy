#include "save.h"
#include <cstring>

const std::vector<uint8_t>& Save::readData() const {
    return data;
}

void Save::writeData(uint8_t* newData, uint32_t count) {
    data.resize(count);
    memcpy(data.data(), newData, count);
}

Save::Save(const std::vector<uint8_t>& data) :
    data(data) {
}
