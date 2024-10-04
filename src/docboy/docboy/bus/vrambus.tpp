#include "docboy/memory/vram0.h"

template<Device::Type Dev>
uint8_t VramBus::read_vram0(uint16_t vram_address) const {
    return vram0[vram_address];
}

#ifdef ENABLE_CGB
template<Device::Type Dev>
uint8_t VramBus::read_vram1(uint16_t vram_address) const {
    return vram1[vram_address];
}
#endif

template<Device::Type Dev>
uint8_t VramBus::View<Dev>::read_vram0(uint16_t address) const {
    return this->bus.template read_vram0<Dev>(address);
}

#ifdef ENABLE_CGB
template<Device::Type Dev>
uint8_t VramBus::View<Dev>::read_vram1(uint16_t address) const {
    return this->bus.template read_vram1<Dev>(address);
}
#endif