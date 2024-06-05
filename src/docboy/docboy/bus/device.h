#ifndef DEVICE_H
#define DEVICE_H

#include <cstdint>

struct Device {
    using Type = uint8_t;
    static constexpr Type Cpu = 0;
    static constexpr Type Dma = 1;
    static constexpr Type Ppu = 2;
}; // namespace Device

#endif // DEVICE_H