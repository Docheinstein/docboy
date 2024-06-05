#include "docboy/memory/vram.h"

template<Device::Type Dev>
uint8_t VramBus::read(uint16_t vram_address) const {
    return vram[vram_address];
}

template<Device::Type Dev>
uint8_t VramBus::View<Dev>::read(uint16_t address) const {
    return this->bus.template read<Dev>(address);
}