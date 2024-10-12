#include "docboy/memory/vram.h"

template<Device::Type Dev, uint8_t Bank>
uint8_t VramBus::read(uint16_t vram_address) const {
#ifdef ENABLE_CGB
    static_assert(Bank < 2);
#else
    static_assert(Bank < 1);
#endif
    return vram[Bank][vram_address];
}


template<Device::Type Dev>
template<uint8_t Bank>
uint8_t VramBus::View<Dev>::read(uint16_t address) const {
    return this->bus.template read<Dev, Bank>(address);
}
