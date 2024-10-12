#ifndef VRAMBUS_H
#define VRAMBUS_H

#include "docboy/bus/videobus.h"
#include "docboy/memory/fwd/vramfwd.h"

class VramBus final : public VideoBus<VramBus> {
public:
    template <Device::Type Dev>
    class View : public VideoBus::View<Dev> {
    public:
        /* implicit */ View(VramBus& bus) :
            VideoBusView<VramBus, Dev>(bus) {
        }

        template <uint8_t Bank>
        uint8_t read(uint16_t address) const;
    };

    explicit VramBus(Vram* vram);

    template <Device::Type Dev, uint8_t Bank>
    uint8_t read(uint16_t vram_address) const;

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

#ifdef ENABLE_CGB
    void set_vram_bank(bool bank);
#endif

private:
#if ENABLE_CGB
    uint8_t read_vram(uint16_t address) const;
    void write_vram(uint16_t address, uint8_t value);
#endif

    Vram* vram;

#ifdef ENABLE_CGB
    bool vram_bank {};
#endif
};

#include "docboy/bus/vrambus.tpp"

#endif // VRAMBUS_H