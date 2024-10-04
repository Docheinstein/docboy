#ifndef VRAMBUS_H
#define VRAMBUS_H

#include "docboy/bus/videobus.h"
#include "docboy/memory/fwd/vram0fwd.h"
#include "docboy/memory/fwd/vram1fwd.h"

class VramBus final : public VideoBus<VramBus> {
public:
    template <Device::Type Dev>
    class View : public VideoBus::View<Dev> {
    public:
        /* implicit */ View(VramBus& bus) :
            VideoBusView<VramBus, Dev>(bus) {
        }

        uint8_t read_vram0(uint16_t address) const;
#ifdef ENABLE_CGB
        uint8_t read_vram1(uint16_t address) const;
#endif
    };

#ifdef ENABLE_CGB
    VramBus(Vram0& vram0, Vram1& vram1);
#else
    explicit VramBus(Vram0& vram);
#endif

    template <Device::Type Dev>
    uint8_t read_vram0(uint16_t vram_address) const;

#ifdef ENABLE_CGB
    template <Device::Type Dev>
    uint8_t read_vram1(uint16_t vram_address) const;
#endif

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

#ifdef ENABLE_CGB
    void switch_bank(bool bank);
#endif

private:
#if ENABLE_CGB
    uint8_t read_vram(uint16_t address) const;
    void write_vram(uint16_t address, uint8_t value);
#endif

    Vram0& vram0;

#ifdef ENABLE_CGB
    Vram1& vram1;

    bool vram_bank {};
#endif
};

#include "docboy/bus/vrambus.tpp"

#endif // VRAMBUS_H