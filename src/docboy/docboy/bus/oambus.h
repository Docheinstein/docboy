#ifndef OAMBUS_H
#define OAMBUS_H

#include "docboy/bus/videobus.h"

class NotUsable;
class Oam;

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

#ifdef ENABLE_CGB
    OamBus(Oam& oam, NotUsable& not_usable);
#else
    explicit OamBus(Oam& oam);
#endif

    template <Device::Type Dev>
    void flush_write_request(uint8_t value);

    template <Device::Type Dev>
    void clear_write_request();

    template <Device::Type Dev>
    void read_word_request(uint16_t addr);

    template <Device::Type Dev>
    Word flush_read_word_request();

    void tick_t2();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
#ifndef ENABLE_CGB
    uint16_t read_word(uint8_t) const;
    void write_word(uint8_t row_addr, uint16_t word);

    void write_row(uint8_t row_addr, uint16_t w0, uint16_t w1, uint16_t w2, uint16_t w3);

    // OAM corruption bugs: read
    void oam_bug_read(uint8_t row_addr);

    void oam_bug_read_00();

    void oam_bug_read_9c_6();
    void oam_bug_read_9c_2();
    void oam_bug_read_9c_0_4();

    // OAM corruption bugs: write
    void oam_bug_write(uint8_t row_addr);

    void oam_bug_write_00_0();
    void oam_bug_write_00_2();
    void oam_bug_write_00_3();
    void oam_bug_write_00_4();
    void oam_bug_write_00_5();
    void oam_bug_write_00_6();
    void oam_bug_write_00_7();

    void oam_bug_write_9c_0();
    void oam_bug_write_9c_2();
    void oam_bug_write_9c_4();
    void oam_bug_write_9c_6();

    // OAM corruption bugs: read/write
    void oam_bug_read_write(uint8_t row_addr);
#endif

    Oam& oam;
#ifdef ENABLE_CGB
    NotUsable& not_usable;
#endif

#ifndef ENABLE_CGB
    struct {
        bool happened {};
        uint8_t previous_data {};
    } mcycle_write;
#endif
};

#include "docboy/bus/oambus.tpp"

#endif // OAMBUS_H
