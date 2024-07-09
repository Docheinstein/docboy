#ifndef APU_H
#define APU_H

#include <cstdint>
#include <functional>

#include "docboy/common/macros.h"
#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"

class Timers;
class Parcel;

class Apu {
    DEBUGGABLE_CLASS()

public:
    static constexpr uint32_t SAMPLES_PER_SECOND = 32768;
    static constexpr uint32_t TICKS_BETWEEN_SAMPLES = Specs::Frequencies::CLOCK / SAMPLES_PER_SECOND;
    static constexpr uint32_t SAMPLES_PER_FRAME =
        SAMPLES_PER_SECOND * Specs::Ppu::DOTS_PER_FRAME / Specs::Frequencies::CLOCK;

    explicit Apu(Timers& timers);

    void set_volume(float volume /* [0:1]*/);

    void set_audio_callback(std::function<void(const int16_t* samples, uint32_t count)>&& callback);

    void tick_t0();
    void tick_t1();
    void tick_t2();
    void tick_t3();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    // I/O registers

    uint8_t read_nr10() const;
    void write_nr10(uint8_t value);

    uint8_t read_nr11() const;
    void write_nr11(uint8_t value);

    uint8_t read_nr12() const;
    void write_nr12(uint8_t value);

    uint8_t read_nr14() const;
    void write_nr14(uint8_t value);

    uint8_t read_nr21() const;
    void write_nr21(uint8_t value);

    uint8_t read_nr22() const;
    void write_nr22(uint8_t value);

    uint8_t read_nr24() const;
    void write_nr24(uint8_t value);

    void write_nr30(uint8_t value);
    void write_nr32(uint8_t value);
    void write_nr41(uint8_t value);
    void write_nr44(uint8_t value);

    uint8_t read_nr52() const;
    void write_nr52(uint8_t value);

    struct Nr10 : Composite<Specs::Registers::Sound::NR11> {
        UInt8 pace {make_uint8()};
        Bool direction {make_bool()};
        UInt8 step {make_uint8()};
    } nr10 {};
    struct Nr11 : Composite<Specs::Registers::Sound::NR11> {
        UInt8 duty_cycle {make_uint8()};
        UInt8 initial_length_timer {make_uint8()};
    } nr11 {};
    struct Nr12 : Composite<Specs::Registers::Sound::NR12> {
        UInt8 initial_volume {make_uint8()};
        Bool envelope_direction {make_bool()};
        UInt8 sweep_pace {make_uint8()};
    } nr12 {};
    UInt8 nr13 {make_uint8(Specs::Registers::Sound::NR13)};
    struct Nr14 : Composite<Specs::Registers::Sound::NR14> {
        Bool trigger {make_bool()};
        Bool length_enable {make_bool()};
        UInt8 period {make_uint8()};
    } nr14 {};
    struct Nr21 : Composite<Specs::Registers::Sound::NR21> {
        UInt8 duty_cycle {make_uint8()};
        UInt8 initial_length_timer {make_uint8()};
    } nr21 {};
    struct Nr22 : Composite<Specs::Registers::Sound::NR22> {
        UInt8 initial_volume {make_uint8()};
        Bool envelope_direction {make_bool()};
        UInt8 sweep_pace {make_uint8()};
    } nr22 {};
    UInt8 nr23 {make_uint8(Specs::Registers::Sound::NR23)};
    struct Nr24 : Composite<Specs::Registers::Sound::NR24> {
        Bool trigger {make_bool()};
        Bool length_enable {make_bool()};
        UInt8 period {make_uint8()};
    } nr24 {};
    UInt8 nr30 {make_uint8(Specs::Registers::Sound::NR30)};
    UInt8 nr31 {make_uint8(Specs::Registers::Sound::NR31)};
    UInt8 nr32 {make_uint8(Specs::Registers::Sound::NR32)};
    UInt8 nr33 {make_uint8(Specs::Registers::Sound::NR33)};
    UInt8 nr34 {make_uint8(Specs::Registers::Sound::NR34)};
    UInt8 nr41 {make_uint8(Specs::Registers::Sound::NR41)};
    UInt8 nr42 {make_uint8(Specs::Registers::Sound::NR42)};
    UInt8 nr43 {make_uint8(Specs::Registers::Sound::NR43)};
    UInt8 nr44 {make_uint8(Specs::Registers::Sound::NR44)};
    UInt8 nr50 {make_uint8(Specs::Registers::Sound::NR50)};
    UInt8 nr51 {make_uint8(Specs::Registers::Sound::NR51)};
    struct Nr52 : Composite<Specs::Registers::Sound::NR52> {
        Bool enable {make_bool()};
        Bool ch4 {make_bool()};
        Bool ch3 {make_bool()};
        Bool ch2 {make_bool()};
        Bool ch1 {make_bool()};
    } nr52 {};
    UInt8 wave0 {make_uint8(Specs::Registers::Sound::WAVE0)};
    UInt8 wave1 {make_uint8(Specs::Registers::Sound::WAVE1)};
    UInt8 wave2 {make_uint8(Specs::Registers::Sound::WAVE2)};
    UInt8 wave3 {make_uint8(Specs::Registers::Sound::WAVE3)};
    UInt8 wave4 {make_uint8(Specs::Registers::Sound::WAVE4)};
    UInt8 wave5 {make_uint8(Specs::Registers::Sound::WAVE5)};
    UInt8 wave6 {make_uint8(Specs::Registers::Sound::WAVE6)};
    UInt8 wave7 {make_uint8(Specs::Registers::Sound::WAVE7)};
    UInt8 wave8 {make_uint8(Specs::Registers::Sound::WAVE8)};
    UInt8 wave9 {make_uint8(Specs::Registers::Sound::WAVE9)};
    UInt8 waveA {make_uint8(Specs::Registers::Sound::WAVEA)};
    UInt8 waveB {make_uint8(Specs::Registers::Sound::WAVEB)};
    UInt8 waveC {make_uint8(Specs::Registers::Sound::WAVEC)};
    UInt8 waveD {make_uint8(Specs::Registers::Sound::WAVED)};
    UInt8 waveE {make_uint8(Specs::Registers::Sound::WAVEE)};
    UInt8 waveF {make_uint8(Specs::Registers::Sound::WAVEF)};

private:
    void turn_on();
    void turn_off();

    int16_t compute_audio_sample() const;

    Timers& timers;

    int16_t samples[SAMPLES_PER_FRAME] {};

    float volume {1.0f};

    std::function<void(const int16_t*, uint32_t)> audio_callback {};

    struct {
        bool dac {};
        uint8_t length_timer {};
        uint16_t period_timer {};

        uint8_t envelope_counter {};

        // TODO: are these loaded from NR12 after a trigger? Not sure.
        uint8_t volume {};
        bool envelope_direction {};
        uint8_t sweep_pace {};

        uint8_t sweep_counter {};

        uint8_t square_wave_position {};
    } ch1 {};

    struct {
        bool dac {};
        uint8_t length_timer {};
        uint16_t period_timer {};

        uint8_t envelope_counter {};

        // TODO: are these loaded from NR22 after a trigger? Not sure.
        uint8_t volume {};
        bool envelope_direction {};
        uint8_t sweep_pace {};

        uint8_t square_wave_position {};
    } ch2 {};

    uint16_t prev_div_bit_4 {};
    uint16_t div_apu {};

    uint16_t sample_index {0}; // [0, SAMPLES_PER_FRAME)

    uint16_t ticks_since_last_sample {}; // [0, TICKS_BETWEEN_SAMPLES)
};

#endif // APU_H
