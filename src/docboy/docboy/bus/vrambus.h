#ifndef VRAMBUS_H
#define VRAMBUS_H

#include "docboy/memory/vram.h"
#include "videobus.h"

class VramBus : public VideoBus<VramBus> {
public:
    template <Device::Type Dev>
    class View : public VideoBus::View<Dev> {
    public:
        View(VramBus& bus) :
            VideoBusView<VramBus, Dev>(bus) {
        }

        [[nodiscard]] uint8_t read(uint16_t address) const;
    };

    explicit VramBus(Vram& vram);

    template <Device::Type Dev>
    [[nodiscard]] uint8_t read(uint16_t vramAddress) const;

private:
    [[nodiscard]] uint8_t readVram(uint16_t address) const;
    void writeVram(uint16_t address, uint8_t value);

    Vram& vram;
};

#include "vrambus.tpp"

#endif // VRAMBUS_H