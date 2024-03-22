#ifndef OAMBUS_H
#define OAMBUS_H

#include "docboy/memory/oam.h"
#include "videobus.h"

class OamBus : public VideoBus<OamBus> {

public:
    struct Word {
        uint8_t a;
        uint8_t b;
    };

    template <Device::Type Dev>
    class View : public VideoBus::View<Dev> {
    public:
        View(OamBus& bus) :
            VideoBusView<OamBus, Dev>(bus) {
        }

        void readWordRequest(uint16_t addr);
        Word flushReadWordRequest();
    };

    explicit OamBus(Oam& oam);

    template <Device::Type Dev>
    void clearWriteRequest();

    template <Device::Type Dev>
    void readWordRequest(uint16_t addr);

    template <Device::Type Dev>
    Word flushReadWordRequest();

private:
    [[nodiscard]] uint8_t readOam(uint16_t address) const;
    void writeOam(uint16_t address, uint8_t value);

    [[nodiscard]] uint8_t readFF(uint16_t address) const;
    void writeNop(uint16_t address, uint8_t value);

    Oam& oam;
};

template <Device::Type Dev>
void OamBus::View<Dev>::readWordRequest(uint16_t addr) {
    this->bus.template readWordRequest<Dev>(addr);
}

template <Device::Type Dev>
OamBus::Word OamBus::View<Dev>::flushReadWordRequest() {
    return this->bus.template flushReadWordRequest<Dev>();
}

#include "oambus.tpp"

#endif // OAMBUS_H
