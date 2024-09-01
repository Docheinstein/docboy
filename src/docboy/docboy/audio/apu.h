#ifndef APU_H
#define APU_H

#include <cstdint>
#include <functional>

#include "docboy/common/macros.h"
#include "docboy/common/specs.h"
#include "docboy/memory/cell.h"
#include "docboy/memory/memory.h"

class Timers;
class Parcel;

class Apu {
    DEBUGGABLE_CLASS()

public:
    struct AudioSample {
        int16_t left;
        int16_t right;
    };

    static constexpr uint32_t NUM_CHANNELS = 2;

    explicit Apu(Timers& timers);

    void set_volume(float volume /* [0:1]*/);

    void set_sample_rate(double rate);

    void set_audio_sample_callback(std::function<void(const AudioSample sample)>&& callback);

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

    uint8_t read_nr13() const;
    void write_nr13(uint8_t value);

    uint8_t read_nr14() const;
    void write_nr14(uint8_t value);

    uint8_t read_nr21() const;
    void write_nr21(uint8_t value);

    uint8_t read_nr22() const;
    void write_nr22(uint8_t value);

    uint8_t read_nr23() const;
    void write_nr23(uint8_t value);

    uint8_t read_nr24() const;
    void write_nr24(uint8_t value);

    uint8_t read_nr30() const;
    void write_nr30(uint8_t value);

    uint8_t read_nr31() const;
    void write_nr31(uint8_t value);

    uint8_t read_nr32() const;
    void write_nr32(uint8_t value);

    uint8_t read_nr33() const;
    void write_nr33(uint8_t value);

    uint8_t read_nr34() const;
    void write_nr34(uint8_t value);

    uint8_t read_nr41() const;
    void write_nr41(uint8_t value);

    uint8_t read_nr42() const;
    void write_nr42(uint8_t value);

    uint8_t read_nr43() const;
    void write_nr43(uint8_t value);

    uint8_t read_nr44() const;
    void write_nr44(uint8_t value);

    uint8_t read_nr50() const;
    void write_nr50(uint8_t value);

    uint8_t read_nr51() const;
    void write_nr51(uint8_t value);

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
    struct Nr13 {
        UInt8 period_low {make_uint8(Specs::Registers::Sound::NR13)};
    } nr13;
    struct Nr14 : Composite<Specs::Registers::Sound::NR14> {
        Bool trigger {make_bool()};
        Bool length_enable {make_bool()};
        UInt8 period_high {make_uint8()};
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
    struct Nr23 {
        UInt8 period_low {make_uint8(Specs::Registers::Sound::NR23)};
    } nr23;
    struct Nr24 : Composite<Specs::Registers::Sound::NR24> {
        Bool trigger {make_bool()};
        Bool length_enable {make_bool()};
        UInt8 period_high {make_uint8()};
    } nr24 {};
    struct Nr30 : Composite<Specs::Registers::Sound::NR30> {
        Bool dac {make_bool()};
    } nr30 {};
    struct Nr31 {
        UInt8 initial_length_timer {make_uint8(Specs::Registers::Sound::NR31)};
    } nr31;
    struct Nr32 : Composite<Specs::Registers::Sound::NR32> {
        UInt8 volume {make_uint8()};
    } nr32 {};
    struct Nr33 {
        UInt8 period_low {make_uint8(Specs::Registers::Sound::NR33)};
    } nr33;
    struct Nr34 : Composite<Specs::Registers::Sound::NR34> {
        Bool trigger {make_bool()};
        Bool length_enable {make_bool()};
        UInt8 period_high {make_uint8()};
    } nr34 {};
    struct Nr41 : Composite<Specs::Registers::Sound::NR41> {
        UInt8 initial_length_timer {make_uint8()};
    } nr41 {};
    struct Nr42 : Composite<Specs::Registers::Sound::NR42> {
        UInt8 initial_volume {make_uint8()};
        Bool envelope_direction {make_bool()};
        UInt8 sweep_pace {make_uint8()};
    } nr42 {};
    struct Nr43 : Composite<Specs::Registers::Sound::NR43> {
        UInt8 clock_shift {make_uint8()};
        Bool lfsr_width {make_bool()};
        UInt8 clock_divider {make_uint8()};
    } nr43 {};
    struct Nr44 : Composite<Specs::Registers::Sound::NR44> {
        Bool trigger {make_bool()};
        Bool length_enable {make_bool()};
    } nr44 {};
    struct Nr50 : Composite<Specs::Registers::Sound::NR50> {
        Bool vin_left {make_bool()};
        uint8_t volume_left {make_uint8()};
        Bool vin_right {make_bool()};
        uint8_t volume_right {make_uint8()};
    } nr50 {};
    struct Nr51 : Composite<Specs::Registers::Sound::NR51> {
        Bool ch4_left {make_bool()};
        Bool ch3_left {make_bool()};
        Bool ch2_left {make_bool()};
        Bool ch1_left {make_bool()};
        Bool ch4_right {make_bool()};
        Bool ch3_right {make_bool()};
        Bool ch2_right {make_bool()};
        Bool ch1_right {make_bool()};
    } nr51 {};
    struct Nr52 : Composite<Specs::Registers::Sound::NR52> {
        Bool enable {make_bool()};
        Bool ch4 {make_bool()};
        Bool ch3 {make_bool()};
        Bool ch2 {make_bool()};
        Bool ch1 {make_bool()};
    } nr52 {};

    Memory<Specs::Registers::Sound::WAVE0, Specs::Registers::Sound::WAVEF> wave_ram;

private:
    void turn_on();
    void turn_off();

    void tick_sampler();

    AudioSample compute_audio_sample() const;

    uint32_t compute_next_period_sweep_period() const;

    Timers& timers;

    std::function<void(const AudioSample)> audio_sample_callback {};

    float master_volume {1.0f};

    uint64_t ticks {};

    double sample_period {};
    double next_tick_sample {};

    struct {
        bool dac {};

        uint8_t volume {};

        uint8_t length_timer {};

        struct {
            uint8_t position {};
            uint16_t timer {};
        } wave;

        struct {
            bool direction {};
            uint8_t pace {};
            uint8_t timer {};
        } volume_sweep;

        struct {
            bool enabled {};
            uint16_t period {};
            uint8_t timer {};
        } period_sweep;
    } ch1 {};

    struct {
        bool dac {};

        uint8_t volume {};

        uint8_t length_timer {};

        struct {
            uint8_t position {};
            uint16_t timer {};
        } wave;

        struct {
            bool direction {};
            uint8_t pace {};
            uint8_t timer {};
        } volume_sweep;
    } ch2 {};

    struct {
        uint16_t length_timer {};

        struct {
            uint8_t position {};
            uint16_t timer {};
        } wave;
    } ch3 {};

    struct {
        bool dac {};

        uint8_t volume {};

        uint8_t length_timer {};

        struct {
            uint16_t timer {};
        } wave;

        struct {
            bool direction {};
            uint8_t pace {};
            uint8_t timer {};
        } volume_sweep;

        uint16_t lfsr {};
    } ch4 {};

    bool prev_div_bit_4 {};
    uint8_t div_apu {};
};

#endif // APU_H
