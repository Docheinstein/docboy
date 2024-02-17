template<Device::Type Dev>
uint8_t VramBus::read(uint16_t vramAddress) const {
    return vram[vramAddress];
}

template<Device::Type Dev>
uint8_t VramBus::View<Dev>::read(uint16_t address) const {
    return this->bus.template read<Dev>(address);
}