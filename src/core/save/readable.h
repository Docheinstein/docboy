#ifndef READABLESAVE_H
#define READABLESAVE_H

#include <cstdint>
#include <vector>

class IReadableSave {
public:
    virtual ~IReadableSave() = default;

    [[nodiscard]] virtual const std::vector<uint8_t>& readData() const = 0;
};

#endif // READABLESAVE_H