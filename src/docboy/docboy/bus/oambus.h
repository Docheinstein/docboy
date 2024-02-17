#ifndef OAMBUS_H
#define OAMBUS_H

#include "docboy/memory/oam.h"
#include "videobus.h"

class OamBus : public VideoBus<OamBus> {

public:
    struct ReadTwoResult {
        uint8_t a;
        uint8_t b;
    };

    template <Device::Type Dev>
    class View : public VideoBus::View<Dev> {
    public:
        View(OamBus& bus) :
            VideoBusView<OamBus, Dev>(bus) {
        }

        [[nodiscard]] ReadTwoResult readTwo(uint16_t address) const;
    };

    explicit OamBus(Oam& oam);

    template <Device::Type Dev>
    [[nodiscard]] uint8_t read(uint16_t oamAddress) const;

    template <Device::Type Dev>
    [[nodiscard]] ReadTwoResult readTwo(uint16_t oamAddress) const;

private:
    [[nodiscard]] uint8_t readOam(uint16_t address) const;
    void writeOam(uint16_t address, uint8_t value);

    Oam& oam;
};

#include "oambus.tpp"

#endif // OAMBUS_H
