#ifndef DEVICE_H
#define DEVICE_H

#include <cstdint>

struct Device {
    using Type = uint8_t;
    static constexpr Type Cpu = 0;
    static constexpr Type Dma = 1;
#ifdef ENABLE_CGB
    static constexpr Type Hdma = 2;
#endif
    static constexpr Type Ppu = 3;
    static constexpr Type Idu = 4;
}; // namespace Device

#endif // DEVICE_H