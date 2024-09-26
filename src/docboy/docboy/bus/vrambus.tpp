#include "docboy/memory/vram.h"

template<Device::Type Dev>
uint8_t VramBus::read(uint16_t vram_address) const {
#ifdef ENABLE_CGB
    return vram_bank ? vram1[vram_address] : vram0[vram_address];
#else
    return vram0[vram_address];
#endif
}

template<Device::Type Dev>
uint8_t VramBus::View<Dev>::read(uint16_t address) const {
    return this->bus.template read<Dev>(address);
}