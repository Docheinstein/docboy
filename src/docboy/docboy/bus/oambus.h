#ifndef OAMBUS_H
#define OAMBUS_H

#include "docboy/bus/videobus.h"
#include "docboy/memory/fwd/oamfwd.h"

class OamBus final : public VideoBus<OamBus> {
public:
    struct Word {
        uint8_t a;
        uint8_t b;
    };

    template <Device::Type Dev>
    class View : public VideoBus::View<Dev> {
    public:
        /* implicit */ View(OamBus& bus) :
            VideoBusView<OamBus, Dev>(bus) {
        }

        void read_word_request(uint16_t addr);
        Word flush_read_word_request();
    };

    explicit OamBus(Oam& oam);

    template <Device::Type Dev>
    void clear_write_request();

    template <Device::Type Dev>
    void read_word_request(uint16_t addr);

    template <Device::Type Dev>
    Word flush_read_word_request();

private:
    uint8_t read_oam(uint16_t address) const;
    void write_oam(uint16_t address, uint8_t value);

    uint8_t read_ff(uint16_t address) const;
    void write_nop(uint16_t address, uint8_t value);

    Oam& oam;
};

#include "docboy/bus/oambus.tpp"

#endif // OAMBUS_H
