template<Device::Type Dev>
OamBus::ReadTwoResult OamBus::readTwo(uint16_t oamAddress) const {
    check(oamAddress % 2 == 0);

    if constexpr (Dev == Device::Ppu) {
        if (test_bit<W<Device::Dma>>(requests))
            oamAddress = discard_bits<0>(address - Specs::MemoryLayout::OAM::START);
    }

    return {oam[oamAddress], oam[oamAddress + 1]};
}

template<Device::Type Dev>
OamBus::ReadTwoResult OamBus::View<Dev>::readTwo(uint16_t address) const {
    return this->bus.template readTwo<Dev>(address);
}