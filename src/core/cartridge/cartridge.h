#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "cartridgeheader.h"
#include "core/memory/memory.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

class ICartridge : public IMemory {};

class Cartridge : public ICartridge {
public:
    explicit Cartridge(const std::vector<uint8_t>& data);
    explicit Cartridge(std::vector<uint8_t>&& data);

protected:
    std::vector<uint8_t> rom;
};

#endif // CARTRIDGE_H