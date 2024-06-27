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

        uint8_t read(uint16_t address) const;
    };

    explicit VramBus(Vram& vram);

    template <Device::Type Dev>
    uint8_t read(uint16_t vram_address) const;

private:
    Vram& vram;
};

#include "docboy/bus/vrambus.tpp"

#endif // VRAMBUS_H