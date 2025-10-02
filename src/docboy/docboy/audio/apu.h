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
class SpeedSwitchController;

class Apu {
    DEBUGGABLE_CLASS()

public:
    struct AudioSample {
        int16_t left;
        int16_t right;
    };

    static constexpr uint32_t NUM_CHANNELS = 2;

#ifdef ENABLE_CGB
    Apu(Timers& timers, SpeedSwitchController& speed_switch_controller, const uint16_t& pc);
#else
    explicit Apu(Timers& timers, const uint16_t& pc);
#endif

    void set_volume(double volume /* [0:1]*/);
    void set_sample_rate(double rate /* Hz */);
    void set_audio_sample_callback(std::function<void(AudioSample sample)>&& callback);
    void set_high_pass_filter_enabled(bool enabled);

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

    uint8_t read_wave_ram(uint16_t address) const;
    void write_wave_ram(uint16_t address, uint8_t value);

#ifdef ENABLE_CGB
    uint8_t read_pcm12() const;
    uint8_t read_pcm34() const;
#endif

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

#ifdef ENABLE_CGB
    UInt8 pcm12 {make_uint8(Specs::Registers::Sound::PCM12)};
    UInt8 pcm34 {make_uint8(Specs::Registers::Sound::PCM34)};
#endif

private:
    using RegisterUpdater = void (Apu::*)(uint8_t);

    struct DigitalAudioSample {
        uint8_t ch1 {};
        uint8_t ch2 {};
        uint8_t ch3 {};
        uint8_t ch4 {};
    };

    void turn_on();
    void turn_off();

    void tick_even();
    void tick_odd();

    void tick_div_apu();
    void tick_div_apu_falling_edge();
    void tick_div_apu_raising_edge();
    void tick_sampler();

    template <typename Channel, typename ChannelOnFlag, typename Nrx1, typename Nrx3, typename Nrx4>
    void tick_square_wave_channel(Channel& ch, const ChannelOnFlag& ch_on, const Nrx1& nrx1, const Nrx3& nrx3,
                                  const Nrx4& nrx4);
    void tick_wave_channel();
    void tick_noise_channel();

    void tick_period_sweep();
    void tick_period_sweep_reload();
    void tick_period_sweep_recalculation();

    void period_sweep_done();
    void period_sweep_reload_done();
    void period_sweep_recalculation_done();

    void tick_lfsr();

#ifdef ENABLE_CGB
    void update_pcm();
#endif

    uint16_t compute_period_sweep_signed_increment() const;
    uint16_t compute_next_period_sweep_period(uint16_t signed_increment, bool complement_bit) const;

    uint8_t compute_ch1_digital_output() const;
    uint8_t compute_ch2_digital_output() const;
    uint8_t compute_ch3_digital_output() const;
    uint8_t compute_ch4_digital_output() const;
    DigitalAudioSample compute_digital_audio_sample() const;
    AudioSample compute_audio_sample();

    void flush_pending_write();

    void write_register(RegisterUpdater updater, uint8_t value);

    void update_nr10(uint8_t value);
    void update_nr11(uint8_t value);
    void update_nr12(uint8_t value);
    void update_nr13(uint8_t value);
    void update_nr14(uint8_t value);

    void update_nr21(uint8_t value);
    void update_nr22(uint8_t value);
    void update_nr23(uint8_t value);
    void update_nr24(uint8_t value);

    template <typename Channel, typename Nrx1>
    void update_nrx1(Channel& ch, Nrx1& nrx1, uint8_t value);

    template <typename Channel, typename ChannelOnFlag, typename Nrx2>
    void update_nrx2(Channel& ch, ChannelOnFlag& ch_on, Nrx2& nrx2, uint8_t value);

    template <typename Channel, typename Nrx3, typename Nrx4>
    void update_nrx3(Channel& ch, Nrx3& nrx3, Nrx4& nrx4, uint8_t value);

    template <typename Channel, typename ChannelOnFlag, typename Nrx2, typename Nrx3, typename Nrx4>
    void update_nrx4(Channel& ch, ChannelOnFlag& ch_on, Nrx2& nrx2, Nrx3& nrx3, Nrx4& nrx4, uint8_t value);

    void update_nr30(uint8_t value);
    void update_nr31(uint8_t value);
    void update_nr32(uint8_t value);
    void update_nr33(uint8_t value);
    void update_nr34(uint8_t value);

    void update_nr41(uint8_t value);
    void update_nr42(uint8_t value);
    void update_nr43(uint8_t value);
    void update_nr44(uint8_t value);
    void update_nr50(uint8_t value);
    void update_nr51(uint8_t value);
    void update_nr52(uint8_t value);

    template <uint16_t address>
    void update_wave_ram(uint8_t value);

    static const RegisterUpdater REGISTER_UPDATERS[];
    static const RegisterUpdater WAVE_RAM_UPDATERS[];

    Timers& timers;
#ifdef ENABLE_CGB
    // TODO: bad: APU shouldn't know speed_switch_controller
    SpeedSwitchController& speed_switch_controller;
#endif
    const uint16_t& program_counter;

    std::function<void(AudioSample)> audio_sample_callback {};

    double master_volume {1.0f};

    struct {
        uint64_t ticks {};
        double period {};
        double next_tick {};
    } sampling;

    struct {
        bool enabled {};
        double alpha {};
        struct {
            double left {};
            double right {};
        } delta;
    } high_pass_filter;

    uint8_t apu_clock {};
    bool prev_div_edge_bit {};
    uint8_t div_apu {};
#ifdef ENABLE_CGB
    uint8_t div_apu_bit_selector {};
#endif

    struct {
        RegisterUpdater updater {};
        uint8_t value {};
    } pending_write;

    struct {
        bool dac {};

        uint8_t volume {};

        uint8_t length_timer {};

        uint8_t trigger_delay {};

        bool digital_output {};

        bool just_sampled {};

        bool tick_edge {};

        struct {
            uint8_t position {};
            uint16_t timer {};
            uint8_t duty_cycle {};
        } wave;

        struct {
            bool direction {};
            uint8_t countdown {};
            bool reloaded {};
            bool pending_update {};
        } volume_sweep;

        struct {
            uint8_t pace_countdown {};

            uint16_t period {};
            uint16_t increment {};

            uint8_t restart_countdown {};

            struct {
                uint8_t countdown {};
                bool period_reloaded {};
                bool reload_period {};
            } reload;

            struct {
                uint8_t target_trigger_counter {};
                uint8_t trigger_counter {};
                uint8_t countdown {};
                bool instant {};

                uint16_t increment {};

                bool from_trigger {};
            } recalculation;
        } period_sweep;
    } ch1 {};

    struct {
        bool dac {};

        uint8_t volume {};

        uint8_t length_timer {};

        uint8_t trigger_delay {};

        bool digital_output {};

        bool just_sampled {};

        bool tick_edge {};

        struct {
            uint8_t position {};
            uint16_t timer {};
            uint8_t duty_cycle {};
        } wave;

        struct {
            bool direction {};
            uint8_t countdown {};
            bool reloaded {};
            bool pending_update {};
        } volume_sweep;
    } ch2 {};

    struct {
        uint16_t length_timer {};

        uint8_t sample {};

        uint8_t trigger_delay {};

        uint8_t digital_output {};

#ifndef ENABLE_CGB
        bool just_sampled {};
#endif
        struct {
            struct {
                uint8_t byte {};
                bool low_nibble {};
            } position;
            uint16_t timer {};
        } wave;
    } ch3 {};

    struct {
        bool dac {};

        uint8_t volume {};

        uint8_t length_timer {};

        uint8_t trigger_delay {};

        struct {
            bool direction {};
            uint8_t countdown {};
            bool reloaded {};
            bool pending_update {};
        } volume_sweep;

        struct {
            uint16_t lfsr {};

            uint8_t divider_countdown {};
            uint16_t shift_counter {};

            uint8_t divider {};
            uint8_t shift {};

            bool width {};
        } lfsr;
    } ch4 {};
};

#endif // APU_H
