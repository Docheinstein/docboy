#include "docboy/memory/vram.h"

template <Device::Type Dev, uint8_t Bank>
uint8_t VramBus::read(uint16_t vram_address) {
#ifdef ENABLE_CGB
    static_assert(Bank < 2);
#else
    static_assert(Bank < 1);
#endif

    set_bit<Dev>(readers);

    return vram[Bank][vram_address];
}

template <Device::Type Dev>
void VramBus::flush_write_request(uint8_t value) {
#ifdef ENABLE_CGB
    if constexpr (Dev == Device::Hdma) {
        // HDMA fails to write if PPU is reading from VRAM.
        if (!test_bit<W<Device::Hdma>>(requests)) {
            // PPU was reading from VRAM the previous T-cycle.
            return;
        }

        if (test_bit<Device::Ppu>(readers)) {
            // PPU is reading from VRAM this T-cycle.
            reset_bit<W<Device::Hdma>>(requests);
            return;
        }
    }
#endif
    VideoBus<VramBus>::flush_write_request<Dev>(value);
}

template <Device::Type Dev>
void VramBus::write_request(uint16_t addr) {
#ifdef ENABLE_CGB
    if constexpr (Dev == Device::Hdma) {
        // TODO: sometimes write happens and goes to a different address instead.
        if (test_bit<Device::Ppu>(readers)) {
            // HDMA fails to write if PPU is reading from VRAM.
            return;
        }
    }
#endif
    VideoBus<VramBus>::write_request<Dev>(addr);
}

inline void VramBus::tick() {
    readers = 0;
}

template <Device::Type Dev>
template <uint8_t Bank>
uint8_t VramBus::View<Dev>::read(uint16_t address) {
    return this->bus.template read<Dev, Bank>(address);
}
