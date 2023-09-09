#ifndef SAVE_H
#define SAVE_H

#include "readable.h"
#include "writable.h"

class Save : public IReadableSave, public IWritableSave {
public:
    explicit Save() = default;
    explicit Save(const std::vector<uint8_t>& data);

    [[nodiscard]] const std::vector<uint8_t>& readData() const override;
    void writeData(uint8_t* data, uint32_t count) override;

private:
    std::vector<std::uint8_t> data;
};

#endif // SAVE_H