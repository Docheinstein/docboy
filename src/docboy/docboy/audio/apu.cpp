#include "docboy/audio/apu.h"

#include <array>

#include "docboy/bootrom/helpers.h"
#include "docboy/timers/timers.h"

#ifdef ENABLE_CGB
#include "docboy/speedswitch/speedswitchcontroller.h"
#endif

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/parcel.h"

namespace {
constexpr bool SQUARE_WAVES[][8] {
    {false, false, false, false, false, false, false, true}, /* 12.5 % */
    {true, false, false, false, false, false, false, true},  /* 25.0 % */
    {true, false, false, false, false, true, true, true},    /* 50.0 % */
    {false, true, true, true, true, true, true, false}       /* 75.0 % */
};

constexpr std::array<std::array<std::array<std::array<std::array<std::array<uint8_t, 16>, 2>, 2>, 2>, 2>, 2>
generate_nrx2_glitch_table() {
    // TODO: DMG: how does it differ from CGB-E?

    // All the cases can be expressed as a combination of a possible negation and/or an increment/decrement by 1.
    // Here we compute a lookup table for all the cases.

    // [prev_vol != 0][prev_sweep != 0][new_sweep != 0][prev_dir][new_dir]
    constexpr bool GLITCH_NRX2_NEGATE[2][2][2][2][2] = {
        {
            {
                {
                    {/* 0 */ true, /* 1 */ false},
                    {/* 2 */ true, /* 3 */ false},
                },
                {
                    {/* 4 */ true, /* 5 */ false},
                    {/* 6 */ true, /* 7 */ false},
                },
            },
            {
                {
                    {/* 8 */ true, /* 9 */ false},
                    {/* 10 */ true, /* 11 */ false},
                },
                {
                    {/* 12 */ true, /* 13 */ false},
                    {/* 14 */ true, /* 15 */ false},
                },
            },
        },
        {
            {
                {
                    {/* 16 */ false, /* 17 */ true},
                    {/* 18 */ true, /* 19 */ false},
                },
                {
                    {/* 20 */ false, /* 21 */ true},
                    {/* 22 */ true, /* 23 */ false},
                },
            },
            {
                {
                    {/* 24 */ false, /* 25 */ true},
                    {/* 26 */ true, /* 27 */ false},
                },
                {
                    {/* 28 */ false, /* 29 */ true},
                    {/* 30 */ true, /* 31 */ false},
                },
            },
        },
    };

    // [prev_vol != 0][prev_sweep != 0][new_sweep != 0][prev_dir][new_dir]
    constexpr uint8_t GLITCH_NRX2_INCREMENT[2][2][2][2][2] = {
        {
            {
                {
                    {/* 0 */ 1, /* 1 */ 1},
                    {/* 2 */ 1, /* 3 */ 1},
                },
                {
                    {/* 4 */ 0, /* 5 */ 1},
                    {/* 6 */ 0, /* 7 */ 1},
                },
            },
            {
                {
                    {/* 8 */ 1, /* 9 */ 0},
                    {/* 10 */ 1, /* 11 */ 0},
                },
                {
                    {/* 12 */ 1, /* 13 */ 0},
                    {/* 14 */ 1, /* 15 */ 0},
                },
            },
        },
        {
            {
                {
                    {/* 16 */ 0, /* 17 */ 0},
                    {/* 18 */ 1, /* 19 */ 1},
                },
                {
                    {/* 20 */ 15, /* 21 */ 0},
                    {/* 22 */ 0, /* 23 */ 1},
                },
            },
            {
                {
                    {/* 24 */ 0, /* 25 */ 15},
                    {/* 26 */ 1, /* 27 */ 0},
                },
                {
                    {/* 28 */ 0, /* 29 */ 15},
                    {/* 30 */ 1, /* 31 */ 0},
                },
            },
        },
    };

    // [prev_init_volume][prev_sweep][new_sweep][prev_dir][new_dir][prev_vol]
    std::array<std::array<std::array<std::array<std::array<std::array<uint8_t, 16>, 2>, 2>, 2>, 2>, 2> table {};

    for (uint8_t prev_init_volume = 0; prev_init_volume < 2; prev_init_volume++) {
        for (uint8_t prev_sweep = 0; prev_sweep < 2; prev_sweep++) {
            for (uint8_t new_sweep = 0; new_sweep < 2; new_sweep++) {
                for (uint8_t prev_dir = 0; prev_dir < 2; prev_dir++) {
                    for (uint8_t new_dir = 0; new_dir < 2; new_dir++) {
                        for (uint8_t prev_vol = 0; prev_vol < 16; prev_vol++) {
                            const bool negate =
                                GLITCH_NRX2_NEGATE[prev_init_volume][prev_sweep][new_sweep][prev_dir][new_dir];
                            const uint8_t increment =
                                GLITCH_NRX2_INCREMENT[prev_init_volume][prev_sweep][new_sweep][prev_dir][new_dir];

                            const uint8_t vol = mod<16>((negate ? ~prev_vol : prev_vol) + increment);

                            table[prev_init_volume][prev_sweep][new_sweep][prev_dir][new_dir][prev_vol] = vol;
                        }
                    }
                }
            }
        }
    }

    return table;
}

inline int16_t digital_to_analog_volume(uint8_t digital_volume) {
    ASSERT(digital_volume <= 0xF);
    int32_t analog_volume = INT16_MAX - (UINT16_MAX / 15) * digital_volume;
    ASSERT(analog_volume >= INT16_MIN && analog_volume <= INT16_MAX);
    return static_cast<int16_t>(analog_volume);
}

inline int16_t scale_analog_volume_by_master_volume(int16_t input_analog_volume, uint8_t master_volume) {
    ASSERT(master_volume < 8);
    int32_t output_analog_volume = input_analog_volume * (1 + master_volume) / 8;
    ASSERT(output_analog_volume >= INT16_MIN && output_analog_volume <= INT16_MAX);
    return static_cast<int16_t>(output_analog_volume);
}

template <typename Channel, typename ChannelOnFlag>
inline void tick_length_timer(Channel& ch, ChannelOnFlag& ch_on) {
    if (ch.length_timer > 0) {
        if (--ch.length_timer == 0) {
            // Length timer expired: turn off the channel
            ch_on = false;
        }
    }
}

template <typename Channel, typename ChannelOnFlag, typename Nrx4>
inline void tick_length_timer(Channel& ch, ChannelOnFlag& ch_on, const Nrx4& nrx4) {
    if (nrx4.length_enable) {
        tick_length_timer(ch, ch_on);
    }
}

template <typename Channel, typename ChannelOnFlag>
inline void tick_volume_sweep(Channel& ch, const ChannelOnFlag& ch_on) {
    if (ch_on && !ch.volume_sweep.pending_update) {
        // Countdown is decreased even if volume sweep pace is 0.
        // This has meaningful implications if volume sweep is updated while
        // the channel is on (without a retrigger to reload volume sweep).
        ch.volume_sweep.countdown = mod<8>(ch.volume_sweep.countdown - 1);
    }
}

template <typename Channel, typename Nrx2>
inline void handle_volume_sweep_update(Channel& ch, const Nrx2& nrx2) {
    if (ch.volume_sweep.pending_update) {
        // Either increase or the decrease the volume,
        // accordingly to the current volume sweep direction.
        ch.volume_sweep.pending_update = false;
        if (nrx2.envelope_direction) {
            if (ch.volume < 0xF) {
                ++ch.volume;
            }
        } else {
            if (ch.volume > 0) {
                --ch.volume;
            }
        }
    }
}

template <typename Channel, typename Nrx2>
inline void tick_volume_sweep_reload(Channel& ch, const Nrx2& nrx2) {
    if (ch.volume_sweep.countdown == 0) {
        // The volume sweep pace is reloaded with the volume sweep from NRX2.
        // A non-zero sweep pace triggers a volume update for the next DIV_APU tick.
        // This happens regardless the new pace value (it just have to be positive).
        ch.volume_sweep.countdown = nrx2.sweep_pace;
        if (ch.volume_sweep.countdown) {
            ch.volume_sweep.pending_update = true;
        }
    }
}

template <typename Channel>
inline void advance_square_wave_position(Channel& ch) {
    // Advance square wave position
    ch.wave.position = mod<8>(ch.wave.position + 1);

    // Update current output
    ch.digital_output = SQUARE_WAVES[ch.wave.duty_cycle][ch.wave.position];
}

template <typename Channel, typename ChannelOnFlag, typename Nrx1, typename Nrx3, typename Nrx4>
inline void tick_square_wave(Channel& ch, const ChannelOnFlag& ch_on, const Nrx1& nrx1, const Nrx3& nrx3,
                             const Nrx4& nrx4) {
    ch.just_sampled = false;

    ch.tick_edge = !ch.tick_edge;

    if (ch_on && ch.tick_edge) {
        // Wave position does not change immediately after channel's trigger.
        if (!ch.trigger_delay) {
            if (++ch.wave.timer == 2048) {
                ASSERT(ch.wave.duty_cycle < 4);
                ASSERT(ch.wave.position < 8);

                // Advance square wave position
                advance_square_wave_position(ch);

                // Reload period timer
                ch.wave.timer = concat(nrx4.period_high, nrx3.period_low);
                ASSERT(ch.wave.timer < 2048);

                ch.just_sampled = true;
            }
        } else {
            --ch.trigger_delay;
        }

        // Reload duty cycle.
        // Note: there's a window in which, if the channel samples, it outputs accordingly to the "old" duty cycle.
        ch.wave.duty_cycle = nrx1.duty_cycle;
    }
}
} // namespace

const Apu::RegisterUpdater Apu::REGISTER_UPDATERS[] {&Apu::update_nr10,
                                                     &Apu::update_nr11,
                                                     &Apu::update_nr12,
                                                     &Apu::update_nr13,
                                                     &Apu::update_nr14,
                                                     &Apu::update_nr21,
                                                     &Apu::update_nr22,
                                                     &Apu::update_nr23,
                                                     &Apu::update_nr24,
                                                     &Apu::update_nr30,
                                                     &Apu::update_nr31,
                                                     &Apu::update_nr32,
                                                     &Apu::update_nr33,
                                                     &Apu::update_nr34,
                                                     &Apu::update_nr41,
                                                     &Apu::update_nr42,
                                                     &Apu::update_nr43,
                                                     &Apu::update_nr44,
                                                     &Apu::update_nr50,
                                                     &Apu::update_nr51,
                                                     &Apu::update_nr52,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVE0>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVE1>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVE2>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVE3>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVE4>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVE5>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVE6>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVE7>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVE8>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVE9>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVEA>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVEB>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVEC>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVED>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVEE>,
                                                     &Apu::update_wave_ram<Specs::Registers::Sound::WAVEF>};

const Apu::RegisterUpdater Apu::WAVE_RAM_UPDATERS[] {
    &Apu::update_wave_ram<Specs::Registers::Sound::WAVE0>, &Apu::update_wave_ram<Specs::Registers::Sound::WAVE1>,
    &Apu::update_wave_ram<Specs::Registers::Sound::WAVE2>, &Apu::update_wave_ram<Specs::Registers::Sound::WAVE3>,
    &Apu::update_wave_ram<Specs::Registers::Sound::WAVE4>, &Apu::update_wave_ram<Specs::Registers::Sound::WAVE5>,
    &Apu::update_wave_ram<Specs::Registers::Sound::WAVE6>, &Apu::update_wave_ram<Specs::Registers::Sound::WAVE7>,
    &Apu::update_wave_ram<Specs::Registers::Sound::WAVE8>, &Apu::update_wave_ram<Specs::Registers::Sound::WAVE9>,
    &Apu::update_wave_ram<Specs::Registers::Sound::WAVEA>, &Apu::update_wave_ram<Specs::Registers::Sound::WAVEB>,
    &Apu::update_wave_ram<Specs::Registers::Sound::WAVEC>, &Apu::update_wave_ram<Specs::Registers::Sound::WAVED>,
    &Apu::update_wave_ram<Specs::Registers::Sound::WAVEE>, &Apu::update_wave_ram<Specs::Registers::Sound::WAVEF>,
};

#ifdef ENABLE_CGB
Apu::Apu(Timers& timers, SpeedSwitchController& speed_switch_controller, const uint16_t& pc) :
    timers {timers},
    speed_switch_controller {speed_switch_controller},
    program_counter {pc} {
    reset();
}
#else
Apu::Apu(Timers& timers, const uint16_t& pc) :
    timers {timers},
    program_counter {pc} {
    reset();
}
#endif

void Apu::set_volume(float volume) {
    ASSERT(volume >= 0.0f && volume <= 1.0f);
    master_volume = volume;
}

void Apu::set_sample_rate(double rate) {
    sampling.period = static_cast<double>(Specs::Frequencies::CLOCK) / rate;
}

void Apu::set_audio_sample_callback(std::function<void(const AudioSample)>&& callback) {
    audio_sample_callback = std::move(callback);
}

void Apu::tick_t0() {
    tick_even();
}

void Apu::tick_t1() {
    tick_odd();
}

void Apu::tick_t2() {
    tick_even();
}

void Apu::tick_t3() {
    tick_odd();
}

void Apu::save_state(Parcel& parcel) const {
    parcel.write_uint64(sampling.ticks);
    parcel.write_double(sampling.next_tick);

    parcel.write_bool(apu_clock_edge);
    parcel.write_bool(prev_div_edge_bit);
    parcel.write_uint8(div_apu);

    parcel.write_uint8(nr10.pace);
    parcel.write_bool(nr10.direction);
    parcel.write_uint8(nr10.step);
    parcel.write_uint8(nr11.duty_cycle);
    parcel.write_uint8(nr11.initial_length_timer);
    parcel.write_uint8(nr12.initial_volume);
    parcel.write_bool(nr12.envelope_direction);
    parcel.write_uint8(nr12.sweep_pace);
    parcel.write_uint8(nr13.period_low);
    parcel.write_bool(nr14.trigger);
    parcel.write_bool(nr14.length_enable);
    parcel.write_uint8(nr14.period_high);
    parcel.write_uint8(nr21.duty_cycle);
    parcel.write_uint8(nr21.initial_length_timer);
    parcel.write_uint8(nr22.initial_volume);
    parcel.write_bool(nr22.envelope_direction);
    parcel.write_uint8(nr22.sweep_pace);
    parcel.write_uint8(nr23.period_low);
    parcel.write_bool(nr24.trigger);
    parcel.write_bool(nr24.length_enable);
    parcel.write_uint8(nr24.period_high);
    parcel.write_bool(nr30.dac);
    parcel.write_uint8(nr31.initial_length_timer);
    parcel.write_uint8(nr32.volume);
    parcel.write_uint8(nr33.period_low);
    parcel.write_bool(nr34.trigger);
    parcel.write_bool(nr34.length_enable);
    parcel.write_uint8(nr34.period_high);
    parcel.write_uint8(nr41.initial_length_timer);
    parcel.write_uint8(nr42.initial_volume);
    parcel.write_bool(nr42.envelope_direction);
    parcel.write_uint8(nr42.sweep_pace);
    parcel.write_uint8(nr43.clock_shift);
    parcel.write_bool(nr43.lfsr_width);
    parcel.write_uint8(nr43.clock_divider);
    parcel.write_bool(nr44.trigger);
    parcel.write_bool(nr44.length_enable);
    parcel.write_bool(nr50.vin_left);
    parcel.write_uint8(nr50.volume_left);
    parcel.write_bool(nr50.vin_right);
    parcel.write_uint8(nr50.volume_right);
    parcel.write_bool(nr51.ch4_left);
    parcel.write_bool(nr51.ch3_left);
    parcel.write_bool(nr51.ch2_left);
    parcel.write_bool(nr51.ch1_left);
    parcel.write_bool(nr51.ch4_right);
    parcel.write_bool(nr51.ch3_right);
    parcel.write_bool(nr51.ch2_right);
    parcel.write_bool(nr51.ch1_right);
    parcel.write_bool(nr52.enable);
    parcel.write_bool(nr52.ch4);
    parcel.write_bool(nr52.ch3);
    parcel.write_bool(nr52.ch2);
    parcel.write_bool(nr52.ch1);
    wave_ram.save_state(parcel);

    uint8_t i = 0;
    while (i < (uint8_t)array_size(REGISTER_UPDATERS) && pending_write.updater != REGISTER_UPDATERS[i]) {
        ++i;
    }
    parcel.write_uint8(i);
    parcel.write_uint8(pending_write.value);

    parcel.write_bool(ch1.dac);
    parcel.write_uint8(ch1.volume);
    parcel.write_uint8(ch1.length_timer);
    parcel.write_uint8(ch1.trigger_delay);
    parcel.write_bool(ch1.digital_output);
    parcel.write_bool(ch1.just_sampled);
    parcel.write_bool(ch1.tick_edge);
    parcel.write_uint8(ch1.wave.position);
    parcel.write_uint16(ch1.wave.timer);
    parcel.write_uint8(ch1.wave.duty_cycle);
    parcel.write_bool(ch1.volume_sweep.direction);
    parcel.write_uint8(ch1.volume_sweep.countdown);
    parcel.write_bool(ch1.volume_sweep.pending_update);
    parcel.write_uint8(ch1.period_sweep.pace_countdown);
    parcel.write_uint16(ch1.period_sweep.period);
    parcel.write_uint16(ch1.period_sweep.increment);
    parcel.write_uint8(ch1.period_sweep.restart_countdown);
    parcel.write_bool(ch1.period_sweep.just_reloaded_period);
    parcel.write_bool(ch1.period_sweep.recalculation.clock_edge);
    parcel.write_uint8(ch1.period_sweep.recalculation.target_trigger_counter);
    parcel.write_uint8(ch1.period_sweep.recalculation.trigger_counter);
    parcel.write_uint8(ch1.period_sweep.recalculation.countdown);
    parcel.write_uint16(ch1.period_sweep.recalculation.increment);
    parcel.write_bool(ch1.period_sweep.recalculation.from_trigger);
    parcel.write_bool(ch1.period_sweep.recalculation.step_0);

    parcel.write_bool(ch2.dac);
    parcel.write_uint8(ch2.volume);
    parcel.write_uint8(ch2.length_timer);
    parcel.write_uint8(ch2.trigger_delay);
    parcel.write_bool(ch2.digital_output);
    parcel.write_bool(ch2.just_sampled);
    parcel.write_bool(ch2.tick_edge);
    parcel.write_uint8(ch2.wave.position);
    parcel.write_uint16(ch2.wave.timer);
    parcel.write_uint8(ch2.wave.duty_cycle);
    parcel.write_bool(ch2.volume_sweep.direction);
    parcel.write_uint8(ch2.volume_sweep.countdown);
    parcel.write_bool(ch2.volume_sweep.pending_update);

    parcel.write_uint16(ch3.length_timer);
    parcel.write_uint8(ch3.sample);
    parcel.write_uint8(ch3.trigger_delay);
    parcel.write_uint8(ch3.digital_output);
#ifndef ENABLE_CGB
    parcel.write_bool(ch3.just_sampled);
#endif
    parcel.write_uint8(ch3.wave.position.byte);
    parcel.write_bool(ch3.wave.position.low_nibble);
    parcel.write_uint16(ch3.wave.timer);
    parcel.write_bool(ch4.dac);
    parcel.write_uint8(ch4.volume);
    parcel.write_uint8(ch4.length_timer);
    parcel.write_bool(ch4.tick_edge);
    parcel.write_uint16(ch4.wave.timer);
    parcel.write_bool(ch4.volume_sweep.direction);
    parcel.write_uint8(ch4.volume_sweep.countdown);
    parcel.write_bool(ch4.volume_sweep.pending_update);
    parcel.write_uint16(ch4.lfsr);
}

void Apu::load_state(Parcel& parcel) {
    sampling.ticks = parcel.read_uint64();
    sampling.next_tick = parcel.read_double();

    apu_clock_edge = parcel.read_bool();
    prev_div_edge_bit = parcel.read_bool();
    div_apu = parcel.read_uint8();

    nr10.pace = parcel.read_uint8();
    nr10.direction = parcel.read_bool();
    nr10.step = parcel.read_uint8();
    nr11.duty_cycle = parcel.read_uint8();
    nr11.initial_length_timer = parcel.read_uint8();
    nr12.initial_volume = parcel.read_uint8();
    nr12.envelope_direction = parcel.read_bool();
    nr12.sweep_pace = parcel.read_uint8();
    nr13.period_low = parcel.read_uint8();
    nr14.trigger = parcel.read_bool();
    nr14.length_enable = parcel.read_bool();
    nr14.period_high = parcel.read_uint8();
    nr21.duty_cycle = parcel.read_uint8();
    nr21.initial_length_timer = parcel.read_uint8();
    nr22.initial_volume = parcel.read_uint8();
    nr22.envelope_direction = parcel.read_bool();
    nr22.sweep_pace = parcel.read_uint8();
    nr23.period_low = parcel.read_uint8();
    nr24.trigger = parcel.read_bool();
    nr24.length_enable = parcel.read_bool();
    nr24.period_high = parcel.read_uint8();
    nr30.dac = parcel.read_bool();
    nr31.initial_length_timer = parcel.read_uint8();
    nr32.volume = parcel.read_uint8();
    nr33.period_low = parcel.read_uint8();
    nr34.trigger = parcel.read_bool();
    nr34.length_enable = parcel.read_bool();
    nr34.period_high = parcel.read_uint8();
    nr41.initial_length_timer = parcel.read_uint8();
    nr42.initial_volume = parcel.read_uint8();
    nr42.envelope_direction = parcel.read_bool();
    nr42.sweep_pace = parcel.read_uint8();
    nr43.clock_shift = parcel.read_uint8();
    nr43.lfsr_width = parcel.read_bool();
    nr43.clock_divider = parcel.read_uint8();
    nr44.trigger = parcel.read_bool();
    nr44.length_enable = parcel.read_bool();
    nr50.vin_left = parcel.read_bool();
    nr50.volume_left = parcel.read_uint8();
    nr50.vin_right = parcel.read_bool();
    nr50.volume_right = parcel.read_uint8();
    nr51.ch4_left = parcel.read_bool();
    nr51.ch3_left = parcel.read_bool();
    nr51.ch2_left = parcel.read_bool();
    nr51.ch1_left = parcel.read_bool();
    nr51.ch4_right = parcel.read_bool();
    nr51.ch3_right = parcel.read_bool();
    nr51.ch2_right = parcel.read_bool();
    nr51.ch1_right = parcel.read_bool();
    nr52.enable = parcel.read_bool();
    nr52.ch4 = parcel.read_bool();
    nr52.ch3 = parcel.read_bool();
    nr52.ch2 = parcel.read_bool();
    nr52.ch1 = parcel.read_bool();
    wave_ram.load_state(parcel);

    const uint8_t register_updater_index = parcel.read_uint8();
    pending_write.updater =
        register_updater_index < array_size(REGISTER_UPDATERS) ? REGISTER_UPDATERS[register_updater_index] : nullptr;
    pending_write.value = parcel.read_uint8();

    ch1.dac = parcel.read_bool();
    ch1.volume = parcel.read_uint8();
    ch1.length_timer = parcel.read_uint8();
    ch1.trigger_delay = parcel.read_uint8();
    ch1.digital_output = parcel.read_bool();
    ch1.just_sampled = parcel.read_bool();
    ch1.tick_edge = parcel.read_bool();
    ch1.wave.position = parcel.read_uint8();
    ch1.wave.timer = parcel.read_uint16();
    ch1.wave.duty_cycle = parcel.read_uint8();
    ch1.volume_sweep.direction = parcel.read_bool();
    ch1.volume_sweep.countdown = parcel.read_uint8();
    ch1.volume_sweep.pending_update = parcel.read_bool();
    ch1.period_sweep.pace_countdown = parcel.read_uint8();
    ch1.period_sweep.period = parcel.read_uint16();
    ch1.period_sweep.increment = parcel.read_uint16();
    ch1.period_sweep.restart_countdown = parcel.read_uint8();
    ch1.period_sweep.just_reloaded_period = parcel.read_bool();
    ch1.period_sweep.recalculation.clock_edge = parcel.read_bool();
    ch1.period_sweep.recalculation.target_trigger_counter = parcel.read_uint8();
    ch1.period_sweep.recalculation.trigger_counter = parcel.read_uint8();
    ch1.period_sweep.recalculation.countdown = parcel.read_uint8();
    ch1.period_sweep.recalculation.increment = parcel.read_uint16();
    ch1.period_sweep.recalculation.from_trigger = parcel.read_bool();
    ch1.period_sweep.recalculation.step_0 = parcel.read_bool();

    ch2.dac = parcel.read_bool();
    ch2.volume = parcel.read_uint8();
    ch2.length_timer = parcel.read_uint8();
    ch2.trigger_delay = parcel.read_uint8();
    ch2.digital_output = parcel.read_bool();
    ch2.just_sampled = parcel.read_bool();
    ch2.tick_edge = parcel.read_bool();
    ch2.wave.position = parcel.read_uint8();
    ch2.wave.timer = parcel.read_uint16();
    ch2.wave.duty_cycle = parcel.read_uint8();
    ch2.volume_sweep.direction = parcel.read_bool();
    ch2.volume_sweep.countdown = parcel.read_uint8();
    ch2.volume_sweep.pending_update = parcel.read_bool();

    ch3.length_timer = parcel.read_uint16();
    ch3.sample = parcel.read_uint8();
    ch3.trigger_delay = parcel.read_uint8();
    ch3.digital_output = parcel.read_uint8();
#ifndef ENABLE_CGB
    ch3.just_sampled = parcel.read_bool();
#endif
    ch3.wave.position.byte = parcel.read_uint8();
    ch3.wave.position.low_nibble = parcel.read_bool();
    ch3.wave.timer = parcel.read_uint16();

    ch4.dac = parcel.read_bool();
    ch4.volume = parcel.read_uint8();
    ch4.length_timer = parcel.read_uint8();
    ch4.tick_edge = parcel.read_bool();
    ch4.wave.timer = parcel.read_uint16();
    ch4.volume_sweep.direction = parcel.read_bool();
    ch4.volume_sweep.countdown = parcel.read_uint8();
    ch4.volume_sweep.pending_update = parcel.read_bool();
    ch4.lfsr = parcel.read_uint16();
}

void Apu::reset() {
    sampling.ticks = 0;
    sampling.next_tick = 0.0;

    apu_clock_edge = false;
    prev_div_edge_bit = false;
    div_apu = 2;

    // Reset I/O registers
    nr10.pace = 0;
    nr10.direction = false;
    nr10.step = 0;
    nr11.duty_cycle = if_bootrom_else(0, 0b10);
    nr11.initial_length_timer = if_bootrom_else(0, 0b111111);
    nr12.initial_volume = if_bootrom_else(0, 0b1111);
    nr12.envelope_direction = false;
    nr12.sweep_pace = if_bootrom_else(0, 0b011);
    nr13.period_low = if_bootrom_else(0, 0b11111111);
    nr14.trigger = true;
    nr14.length_enable = false;
    nr14.period_high = 0b111;
    nr21.duty_cycle = 0;
    nr21.initial_length_timer = if_bootrom_else(0, 0b111111);
    nr22.initial_volume = false;
    nr22.envelope_direction = false;
    nr22.sweep_pace = false;
    nr23.period_low = if_bootrom_else(0, 0b11111111);
    nr24.trigger = true;
    nr24.length_enable = false;
    nr24.period_high = 0b111;
    nr30.dac = false;
    nr31.initial_length_timer = if_bootrom_else(0, 0b11111111);
    nr32.volume = 0;
    nr33.period_low = if_bootrom_else(0, 0b11111111);
    nr34.trigger = true;
    nr34.length_enable = false;
    nr34.period_high = 0b111;
    nr41.initial_length_timer = if_bootrom_else(0, 0b111111);
    nr42.initial_volume = 0;
    nr42.envelope_direction = false;
    nr42.sweep_pace = 0;
    nr43.clock_shift = 0;
    nr43.lfsr_width = false;
    nr43.clock_divider = 0;
    nr44.trigger = true;
    nr44.length_enable = false;
    nr50.vin_left = false;
    nr50.volume_left = if_bootrom_else(0, 0b111);
    nr50.vin_right = false;
    nr50.volume_right = if_bootrom_else(0, 0b111);
    nr51.ch4_left = if_bootrom_else(false, true);
    nr51.ch3_left = if_bootrom_else(false, true);
    nr51.ch2_left = if_bootrom_else(false, true);
    nr51.ch1_left = if_bootrom_else(false, true);
    nr51.ch4_right = false;
    nr51.ch3_right = false;
    nr51.ch2_right = if_bootrom_else(false, true);
    nr51.ch1_right = if_bootrom_else(false, true);
    nr52.enable = true;
    nr52.ch4 = false;
    nr52.ch3 = false;
    nr52.ch2 = false;
    nr52.ch1 = true;

    for (uint16_t i = 0; i < decltype(wave_ram)::Size; i++) {
#ifdef ENABLE_CGB
        wave_ram[i] = mod<2>(i) == 0 ? 0x00 : 0xFF;
#else
        wave_ram[i] = 0;
#endif
    }

    pending_write.updater = nullptr;
    pending_write.value = 0;

    // Length timers are not reset
    // [blargg/08-len_ctr_during_power

    ch1.dac = true;
    ch1.volume = 0;
    ch1.length_timer = 0;
    ch1.trigger_delay = 0;
    ch1.digital_output = false;
    ch1.just_sampled = false;
    ch1.tick_edge = false;
    ch1.wave.position = 0;
    ch1.wave.timer = concat(nr14.period_high, nr13.period_low);
    ch1.wave.duty_cycle = 0;
    ch1.volume_sweep.direction = false;
    ch1.volume_sweep.countdown = 0;
    ch1.volume_sweep.pending_update = false;
    ch1.period_sweep.pace_countdown = 0;
    ch1.period_sweep.period = 0;
    ch1.period_sweep.increment = 0;
    ch1.period_sweep.restart_countdown = 0;
    ch1.period_sweep.just_reloaded_period = false;
    ch1.period_sweep.recalculation.clock_edge = false;
    ch1.period_sweep.recalculation.target_trigger_counter = 0;
    ch1.period_sweep.recalculation.trigger_counter = 0;
    ch1.period_sweep.recalculation.countdown = 0;
    ch1.period_sweep.recalculation.increment = 0;
    ch1.period_sweep.recalculation.from_trigger = false;
    ch1.period_sweep.recalculation.step_0 = false;

    ch2.dac = false;
    ch2.volume = 0;
    ch2.length_timer = 0;
    ch2.trigger_delay = 0;
    ch2.digital_output = false;
    ch2.just_sampled = false;
    ch2.tick_edge = false;
    ch2.wave.position = 0;
    ch2.wave.timer = concat(nr24.period_high, nr23.period_low);
    ch2.wave.duty_cycle = 0;
    ch2.volume_sweep.direction = false;
    ch2.volume_sweep.countdown = 0;
    ch2.volume_sweep.pending_update = false;

    ch3.length_timer = 0;
    ch3.sample = 0;
    ch3.trigger_delay = 0;
    ch3.digital_output = 0;
#ifndef ENABLE_CGB
    ch3.just_sampled = false;
#endif
    ch3.wave.position.byte = 0;
    ch3.wave.position.low_nibble = false;
    ch3.wave.timer = 0;

    ch4.dac = false;
    ch4.volume = 0;
    ch4.length_timer = 0;
    ch4.tick_edge = false;
    ch4.wave.timer = 0;
    ch4.volume_sweep.direction = false;
    ch4.volume_sweep.countdown = 0;
    ch4.volume_sweep.pending_update = false;
    ch4.lfsr = 0;
}

uint8_t Apu::read_nr10() const {
    return 0b10000000 | nr10.pace << Specs::Bits::Audio::NR10::PACE |
           nr10.direction << Specs::Bits::Audio::NR10::DIRECTION | nr10.step << Specs::Bits::Audio::NR10::STEP;
}

void Apu::write_nr10(uint8_t value) {
    write_register(&Apu::update_nr10, value);
}

uint8_t Apu::read_nr11() const {
    return 0b00111111 | nr11.duty_cycle << Specs::Bits::Audio::NR11::DUTY_CYCLE;
}

void Apu::write_nr11(uint8_t value) {
    write_register(&Apu::update_nr11, value);
}

uint8_t Apu::read_nr12() const {
    return nr12.initial_volume << Specs::Bits::Audio::NR12::INITIAL_VOLUME |
           nr12.envelope_direction << Specs::Bits::Audio::NR12::ENVELOPE_DIRECTION |
           nr12.sweep_pace << Specs::Bits::Audio::NR12::SWEEP_PACE;
}

void Apu::write_nr12(uint8_t value) {
    write_register(&Apu::update_nr12, value);
}

uint8_t Apu::read_nr13() const {
    return 0xFF;
}

void Apu::write_nr13(uint8_t value) {
    write_register(&Apu::update_nr13, value);
}

uint8_t Apu::read_nr14() const {
    return 0b10111111 | nr14.length_enable << Specs::Bits::Audio::NR14::LENGTH_ENABLE;
}

void Apu::write_nr14(uint8_t value) {
    write_register(&Apu::update_nr14, value);
}

uint8_t Apu::read_nr21() const {
    return 0b00111111 | nr21.duty_cycle << Specs::Bits::Audio::NR21::DUTY_CYCLE;
}

void Apu::write_nr21(uint8_t value) {
    write_register(&Apu::update_nr21, value);
}

uint8_t Apu::read_nr22() const {
    return nr22.initial_volume << Specs::Bits::Audio::NR22::INITIAL_VOLUME |
           nr22.envelope_direction << Specs::Bits::Audio::NR22::ENVELOPE_DIRECTION |
           nr22.sweep_pace << Specs::Bits::Audio::NR22::SWEEP_PACE;
}

void Apu::write_nr22(uint8_t value) {
    write_register(&Apu::update_nr22, value);
}

uint8_t Apu::read_nr23() const {
    return 0xFF;
}

void Apu::write_nr23(uint8_t value) {
    write_register(&Apu::update_nr23, value);
}

uint8_t Apu::read_nr24() const {
    return 0b10111111 | nr24.length_enable << Specs::Bits::Audio::NR24::LENGTH_ENABLE;
}

void Apu::write_nr24(uint8_t value) {
    write_register(&Apu::update_nr24, value);
}

uint8_t Apu::read_nr30() const {
    return 0b01111111 | nr30.dac << Specs::Bits::Audio::NR30::DAC;
}

void Apu::write_nr30(uint8_t value) {
    write_register(&Apu::update_nr30, value);
}

uint8_t Apu::read_nr31() const {
    return 0xFF;
}

void Apu::write_nr31(uint8_t value) {
    write_register(&Apu::update_nr31, value);
}

uint8_t Apu::read_nr32() const {
    return 0b10011111 | nr32.volume << Specs::Bits::Audio::NR32::VOLUME;
}

void Apu::write_nr32(uint8_t value) {
    write_register(&Apu::update_nr32, value);
}

uint8_t Apu::read_nr33() const {
    return 0xFF;
}

void Apu::write_nr33(uint8_t value) {
    write_register(&Apu::update_nr33, value);
}

uint8_t Apu::read_nr34() const {
    return 0b10111111 | nr34.length_enable << Specs::Bits::Audio::NR34::LENGTH_ENABLE;
}

void Apu::write_nr34(uint8_t value) {
    write_register(&Apu::update_nr34, value);
}

uint8_t Apu::read_nr41() const {
    return 0xFF;
}

void Apu::write_nr41(uint8_t value) {
    write_register(&Apu::update_nr41, value);
}

uint8_t Apu::read_nr42() const {
    return nr42.initial_volume << Specs::Bits::Audio::NR42::INITIAL_VOLUME |
           nr42.envelope_direction << Specs::Bits::Audio::NR42::ENVELOPE_DIRECTION |
           nr42.sweep_pace << Specs::Bits::Audio::NR42::SWEEP_PACE;
}

void Apu::write_nr42(uint8_t value) {
    write_register(&Apu::update_nr42, value);
}

uint8_t Apu::read_nr43() const {
    return nr43.clock_shift << Specs::Bits::Audio::NR43::CLOCK_SHIFT |
           nr43.lfsr_width << Specs::Bits::Audio::NR43::LFSR_WIDTH |
           nr43.clock_divider << Specs::Bits::Audio::NR43::CLOCK_DIVIDER;
}

void Apu::write_nr43(uint8_t value) {
    write_register(&Apu::update_nr43, value);
}

uint8_t Apu::read_nr44() const {
    return 0b10111111 | nr44.length_enable << Specs::Bits::Audio::NR44::LENGTH_ENABLE;
}

void Apu::write_nr44(uint8_t value) {
    write_register(&Apu::update_nr44, value);
}

uint8_t Apu::read_nr50() const {
    return nr50.vin_left << Specs::Bits::Audio::NR50::VIN_LEFT |
           nr50.volume_left << Specs::Bits::Audio::NR50::VOLUME_LEFT |
           nr50.vin_right << Specs::Bits::Audio::NR50::VIN_RIGHT |
           nr50.volume_right << Specs::Bits::Audio::NR50::VOLUME_RIGHT;
}

void Apu::write_nr50(uint8_t value) {
    write_register(&Apu::update_nr50, value);
}

uint8_t Apu::read_nr51() const {
    return nr51.ch4_left << Specs::Bits::Audio::NR51::CH4_LEFT | nr51.ch3_left << Specs::Bits::Audio::NR51::CH3_LEFT |
           nr51.ch2_left << Specs::Bits::Audio::NR51::CH2_LEFT | nr51.ch1_left << Specs::Bits::Audio::NR51::CH1_LEFT |
           nr51.ch4_right << Specs::Bits::Audio::NR51::CH4_RIGHT |
           nr51.ch3_right << Specs::Bits::Audio::NR51::CH3_RIGHT |
           nr51.ch2_right << Specs::Bits::Audio::NR51::CH2_RIGHT |
           nr51.ch1_right << Specs::Bits::Audio::NR51::CH1_RIGHT;
}

void Apu::write_nr51(uint8_t value) {
    write_register(&Apu::update_nr51, value);
}

uint8_t Apu::read_nr52() const {
    return 0b01110000 | nr52.enable << Specs::Bits::Audio::NR52::AUDIO_ENABLE |
           nr52.ch4 << Specs::Bits::Audio::NR52::CH4_ENABLE | nr52.ch3 << Specs::Bits::Audio::NR52::CH3_ENABLE |
           nr52.ch2 << Specs::Bits::Audio::NR52::CH2_ENABLE | nr52.ch1 << Specs::Bits::Audio::NR52::CH1_ENABLE;
}

void Apu::write_nr52(uint8_t value) {
    write_register(&Apu::update_nr52, value);
}

uint8_t Apu::read_wave_ram(uint16_t address) const {
    if (!nr52.ch3) {
        // Wave ram is accessed normally if CH3 is off.
        return wave_ram[address - Specs::Registers::Sound::WAVE0];
    }

#ifdef ENABLE_CGB
    // [blargg/09-wave_read_while_on]
    return wave_ram[ch3.wave.position.byte];
#else
    // On DMG, when CH3 is on, reading wave ram yields the byte CH3 is currently
    // reading this T-cycle, or 0xFF if CH3 is not reading anything.
    // [blargg/09-wave_read_while_on]
    if (ch3.just_sampled) {
        return wave_ram[ch3.wave.position.byte];
    }
    return 0xFF;
#endif
}

void Apu::write_wave_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= Specs::Registers::Sound::WAVE0 && address <= Specs::Registers::Sound::WAVEF);
    write_register(WAVE_RAM_UPDATERS[address - Specs::Registers::Sound::WAVE0], value);
}

#ifdef ENABLE_CGB
uint8_t Apu::read_pcm12() const {
    return pcm12;
}

uint8_t Apu::read_pcm34() const {
    return pcm34;
}
#endif

void Apu::turn_on() {
    prev_div_edge_bit = true; // TODO: verify this, also in CGB?
    div_apu = 0;
    apu_clock_edge = true;
}

void Apu::turn_off() {
    nr10.pace = 0;
    nr10.direction = false;
    nr10.step = 0;
    nr11.duty_cycle = 0;
    nr11.initial_length_timer = 0;
    nr12.initial_volume = 0;
    nr12.envelope_direction = false;
    nr12.sweep_pace = 0;
    nr13.period_low = 0;
    nr14.trigger = false;
    nr14.length_enable = false;
    nr14.period_high = 0;
    nr21.duty_cycle = 0;
    nr21.initial_length_timer = 0;
    nr22.initial_volume = 0;
    nr22.envelope_direction = false;
    nr22.sweep_pace = 0;
    nr23.period_low = 0;
    nr24.trigger = false;
    nr24.length_enable = false;
    nr24.period_high = 0;
    nr30.dac = false;
    nr31.initial_length_timer = 0;
    nr32.volume = 0;
    nr33.period_low = 0;
    nr34.trigger = false;
    nr34.length_enable = false;
    nr34.period_high = 0;
    nr41.initial_length_timer = 0;
    nr42.initial_volume = 0;
    nr42.envelope_direction = false;
    nr42.sweep_pace = 0;
    nr43.clock_shift = 0;
    nr43.lfsr_width = false;
    nr43.clock_divider = 0;
    nr44.trigger = false;
    nr44.length_enable = false;
    nr50.vin_left = false;
    nr50.volume_left = 0;
    nr50.vin_right = false;
    nr50.volume_right = 0;
    nr51.ch4_left = false;
    nr51.ch3_left = false;
    nr51.ch2_left = false;
    nr51.ch1_left = false;
    nr51.ch4_right = false;
    nr51.ch3_right = false;
    nr51.ch2_right = false;
    nr51.ch1_right = false;
    nr52.ch4 = false;
    nr52.ch3 = false;
    nr52.ch2 = false;
    nr52.ch1 = false;

    ch1.dac = false;
    ch1.digital_output = false;
    ch1.tick_edge = false;
    ch1.wave.position = 0;
    ch1.volume_sweep.pending_update = false;

    ch2.dac = false;
    ch2.digital_output = false;
    ch2.tick_edge = false;
    ch2.wave.position = 0;
    ch2.volume_sweep.pending_update = false;

    ch3.sample = 0;
    ch3.digital_output = 0;
    ch3.wave.position.byte = 0;
    ch3.wave.position.low_nibble = false;

    ch4.dac = false;
    ch4.tick_edge = false;
    ch4.volume_sweep.pending_update = false;
}

void Apu::tick_even() {
    // In single speed mode, writes to APU seem to be delayed by 1 T-cycle.
    flush_pending_write();

    if (nr52.enable) {
        tick_square_wave(ch1, nr52.ch1, nr11, nr13, nr14);
        tick_square_wave(ch2, nr52.ch2, nr21, nr23, nr24);
        tick_noise();

        apu_clock_edge = !apu_clock_edge;
    }

    tick_sampler();

#ifdef ENABLE_CGB
    // TODO: consider to update PCM only when registers or outputs of channels changes.
    update_pcm();
#endif
}

void Apu::tick_odd() {
    if (nr52.enable) {
        tick_period_sweep_recalculation();

        tick_div_apu();

        tick_wave();
    }

    tick_sampler();

#ifdef ENABLE_CGB
    // TODO: consider to update PCM only when registers or outputs of channels changes.
    update_pcm();
#endif
}

void Apu::tick_div_apu() {
    // Increase DIV-APU each time DIV[4] (or DIV[5] in double speed) has a falling edge (~512Hz).
#ifdef ENABLE_CGB
    bool div_edge_bit {};
    if (speed_switch_controller.is_double_speed_mode()) {
        div_edge_bit = test_bit<5>(timers.read_div());
    } else {
        div_edge_bit = test_bit<4>(timers.read_div());
    }
#else
    const bool div_edge_bit = test_bit<4>(timers.read_div());
#endif

    if (prev_div_edge_bit && !div_edge_bit) {
        div_apu++;
        tick_div_apu_falling_edge();
    } else if (!prev_div_edge_bit && div_edge_bit) {
        tick_div_apu_raising_edge();
    }

    prev_div_edge_bit = div_edge_bit;
}

inline void Apu::tick_div_apu_falling_edge() {
    if (mod<2>(div_apu) == 0) {
        // Update length timer
        tick_length_timer(ch1, nr52.ch1, nr14);
        tick_length_timer(ch2, nr52.ch2, nr24);
        tick_length_timer(ch3, nr52.ch3, nr34); // TODO: is CH3 length timer updated here as well, or in tick_odd?
        tick_length_timer(ch4, nr52.ch4, nr44);

        if (mod<4>(div_apu) == 0) {
            // Update period sweep
            tick_period_sweep();

            if (mod<8>(div_apu) == 0) {
                // Update volume sweep
                tick_volume_sweep(ch1, nr52.ch1);
                tick_volume_sweep(ch2, nr52.ch2);
                tick_volume_sweep(ch4, nr52.ch4);
            }
        }
    }

    // Eventually update channel's volume, if volume sweep pace countdown triggered.
    // This doesn't happen at the same time the volume sweep overflows,
    // but only the next DIV_APU event.
    // [samesuite/div_trigger_volume_10,
    //  samesuite/div_write_trigger_volume,
    //  samesuite/div_write_trigger_volume_10]
    handle_volume_sweep_update(ch1, nr12);
    handle_volume_sweep_update(ch2, nr22);
    handle_volume_sweep_update(ch4, nr42);
}

inline void Apu::tick_div_apu_raising_edge() {
    // Volume sweep pace is reloaded from NRx2 and a volume update is eventually triggered.
    // This does not happen during the main DIV_APU tick, it is delayed by half DIV_APU tick.
    tick_volume_sweep_reload(ch1, nr12);
    tick_volume_sweep_reload(ch2, nr22);
    tick_volume_sweep_reload(ch4, nr42);
}

void Apu::tick_sampler() {
    ASSERT(sampling.period > 0.0);

    if (++sampling.ticks >= static_cast<uint64_t>(sampling.next_tick)) {
        sampling.next_tick += sampling.period;

        if (audio_sample_callback) {
            audio_sample_callback(compute_audio_sample());
        }
    }
}

inline void Apu::tick_period_sweep() {
    if (nr52.ch1) {
        if (keep_bits<3>(--ch1.period_sweep.pace_countdown) == 0) {
            // Pace countdown expired: handle period sweep.
            period_sweep_done();
        }
    }
}

void Apu::tick_period_sweep_recalculation() {
    // Period sweep recalculation ticks only once per M-Cycle.
    // The alignment depends on the time the APU has been turned on.
    if (apu_clock_edge == ch1.period_sweep.recalculation.clock_edge) {
        if (ch1.period_sweep.recalculation.trigger_counter < ch1.period_sweep.recalculation.target_trigger_counter) {
            // Recalculation takes some extra cycles (from 2 to 4) after the trigger.
            ++ch1.period_sweep.recalculation.trigger_counter;
        } else if (ch1.period_sweep.recalculation.countdown) {
            // Recalculation countdown is advanced.
            // Note that it is advanced only if the current step is 0, otherwise it is paused.
            // The only exception is if the period sweep was started with step 0: in that case it ticks anyway.
            if (nr10.step || ch1.period_sweep.recalculation.step_0) {
                if (--ch1.period_sweep.recalculation.countdown == 0) {
                    // Recalculation countdown expired: handle period sweep recalculation.
                    period_sweep_recalculation_done();
                }
            }
        }
    }

    // TODO: is this always ticked, or maybe it's aligned with apu_clock_edge in some manner?
    if (ch1.period_sweep.restart_countdown) {
        --ch1.period_sweep.restart_countdown;
    }

    ch1.period_sweep.just_reloaded_period = false;
}

void Apu::tick_wave() {
#ifndef ENABLE_CGB
    ch3.just_sampled = false;
#endif

    if (nr52.ch3) {
        if (!ch3.trigger_delay) {
            if (++ch3.wave.timer == 2048) {
                // Sample
                // Advance square wave position
                if (ch3.wave.position.low_nibble) {
                    ch3.wave.position.low_nibble = false;
                    ch3.wave.position.byte = mod<16>(ch3.wave.position.byte + 1);
                } else {
                    ch3.wave.position.low_nibble = true;
                }

                ASSERT(ch3.wave.position.byte < decltype(wave_ram)::Size);

                // Update current sample and channel output
                ch3.sample = wave_ram[ch3.wave.position.byte];

                if (ch3.wave.position.low_nibble) {
                    ch3.digital_output = keep_bits<4>(ch3.sample);
                } else {
                    ch3.digital_output = get_bits_range<7, 4>(ch3.sample);
                }

                // Reload period timer
                ch3.wave.timer = concat(nr34.period_high, nr33.period_low);

#ifndef ENABLE_CGB
                ch3.just_sampled = true;
#endif

                ASSERT(ch3.wave.timer < 2048);
            }
        } else {
            --ch3.trigger_delay;
        }
    }
}

void Apu::tick_noise() {
    ch4.tick_edge = !ch4.tick_edge;

    if (nr52.ch4 && ch4.tick_edge) {
        uint8_t D = nr43.clock_divider ? 2 * nr43.clock_divider : 1;

        if (++ch4.wave.timer >= 2 * D << nr43.clock_shift) {
            const bool b = test_bit<0>(ch4.lfsr) == test_bit<1>(ch4.lfsr);
            set_bit<15>(ch4.lfsr, b);
            if (nr43.lfsr_width) {
                set_bit<7>(ch4.lfsr, b);
            }
            ch4.lfsr = ch4.lfsr >> 1;
            ch4.wave.timer = 0;
        }
    }
}

void Apu::period_sweep_done() {
    // Glitch.
    // If period_sweep ticks while a recalculation is pending, strange things happens.
    if (ch1.period_sweep.recalculation.countdown) {
        if (ch1.period_sweep.restart_countdown) {
            // If the channel has just been triggered (but not retriggered), the period seems to be reloaded as well.
            ch1.period_sweep.period = concat(nr14.period_high, nr13.period_low);
        }
        // Increment seems to be reset anyway.
        ch1.period_sweep.increment = 0;
    }

    if (nr10.pace) {
        if (nr10.step) {
            // If step is > 0, the internal wave period (NR14 and NR13) is updated accordingly to the new calculation.
            // Note that the internal period register is not updated here, but only when the recalculation is done.
            const uint16_t period =
                compute_next_period_sweep_period(compute_period_sweep_signed_increment(), nr10.direction);
            nr14.period_high = get_bits_range<10, 8>(period);
            nr13.period_low = keep_bits<8>(period);

            ch1.period_sweep.just_reloaded_period = true;

            // A recalculation is scheduled (if it was not already pending) with a delay that depends on the step value.
            if (ch1.period_sweep.restart_countdown == 0) {
                ch1.period_sweep.recalculation.countdown = 1 + nr10.step;
                ch1.period_sweep.recalculation.clock_edge = false;
                ch1.period_sweep.recalculation.step_0 = false;
            }
        } else {
            // The recalculation happens almost instantly if the step is 0 (if it was not already pending).
            // Furthermore, this seems the only case in which the recalculation is not aligned with the internal APU
            // clock, instead it is aligned with this the time the period sweep is done.
            // TODO. further investigations?
            if (ch1.period_sweep.restart_countdown == 0) {
                ch1.period_sweep.recalculation.countdown = 2;
                ch1.period_sweep.recalculation.clock_edge = !apu_clock_edge;
                ch1.period_sweep.recalculation.step_0 = true;
            }
        }

        // Increment is reloaded accordingly to the step.
        ch1.period_sweep.increment = concat(nr14.period_high, nr13.period_low) >> nr10.step;
    }

    // Pace 0 is reloaded as pace 8.
    // [blargg/05-sweep-details]
    ch1.period_sweep.pace_countdown = nr10.pace ? nr10.pace : 8;
}

void Apu::period_sweep_recalculation_done() {
    // Reload the internal shadow period from the current NR14 and NR13.
    ch1.period_sweep.period = concat(nr14.period_high, nr13.period_low);

    // Compute the new increment.
    ch1.period_sweep.recalculation.increment = compute_period_sweep_signed_increment();

    if (nr10.direction == 0) {
        // If the current direction is increasing, the channel might be disabled due to an overflow.
#ifdef ENABLE_CGB
        const bool complement_bit = nr10.direction;
#else
        // DMG glitch: the complement bit does not depend on the nr10.direction, as expected, instead it's always 1,
        // unless the recalculation is the first one coming from a trigger (NR14 write)
        const bool complement_bit = !ch1.period_sweep.recalculation.from_trigger;
#endif

        const uint16_t period =
            compute_next_period_sweep_period(ch1.period_sweep.recalculation.increment, complement_bit);
        if (period >= 2048) {
            // Overflow on recalculation: disable channel.
            nr52.ch1 = false;
        }
    }

    // ch1.period_sweep.recalculation.target_trigger_counter = 0; // TODO: is this necessary?
    ch1.period_sweep.recalculation.from_trigger = false;
}

#ifdef ENABLE_CGB
void Apu::update_pcm() {
    DigitalAudioSample digital_output = compute_digital_audio_sample();
    pcm12 = digital_output.ch2 << 4 | digital_output.ch1;
    pcm34 = digital_output.ch4 << 4 | digital_output.ch3;
}
#endif

inline uint16_t Apu::compute_period_sweep_signed_increment() const {
    // This yields ch1.period_sweep.increment itself if the direction is increasing, or the 1-complement if it's
    // decreasing. This seems the way the period sweep works internally, for negative direction. Indeed, the increment
    // it's not represented in 2-complement, instead it's represented in 1-complement, to which a magic bit is
    // eventually added. Such bit usually is exactly nr10.direction, which yields to 1-complement + 1, which is exactly
    // 2-complement of the increment, that is the negated increment, but there are glitches in the period sweep which
    // makes such magic bit coming from somewhere else, not from the nr10.direction.
    return ch1.period_sweep.increment ^ (nr10.direction ? bitmask<11> : 0);
}

inline uint16_t Apu::compute_next_period_sweep_period(uint16_t signed_increment, bool complement_bit) const {
    return ch1.period_sweep.period + signed_increment + complement_bit;
}

inline uint8_t Apu::compute_ch1_digital_output() const {
    uint8_t digital_output {};

    if (nr52.ch1) {
        ASSERT(ch1.dac);
        ASSERT(ch1.volume <= 0xF);

        // Scale by volume
        digital_output = ch1.digital_output * ch1.volume;

        ASSERT(digital_output <= 0xF);
    }

    return digital_output;
}

inline uint8_t Apu::compute_ch2_digital_output() const {
    uint8_t digital_output {};

    if (nr52.ch2) {
        ASSERT(ch2.dac);
        ASSERT(ch2.volume <= 0xF);

        // Scale by volume
        digital_output = ch2.digital_output * ch2.volume;

        ASSERT(digital_output <= 0xF);
    }

    return digital_output;
}

inline uint8_t Apu::compute_ch3_digital_output() const {
    uint8_t digital_output {};

    if (nr52.ch3 && nr32.volume) {
        ASSERT(nr30.dac);
        ASSERT(nr32.volume <= 0b11);
        ASSERT(ch3.digital_output <= 0x0F);

        // Scale by volume
        digital_output = ch3.digital_output >> (nr32.volume - 1);

        ASSERT(digital_output <= 0xF);
    }

    return digital_output;
}

inline uint8_t Apu::compute_ch4_digital_output() const {
    uint8_t digital_output {};

    if (nr52.ch4) {
        ASSERT(ch4.dac);
        ASSERT(ch4.volume <= 0xF);

        // Scale by volume
        const bool random_output = test_bit<0>(ch4.lfsr);
        digital_output = random_output ? ch4.volume : 0;

        ASSERT(digital_output <= 0xF);
    }

    return digital_output;
}

Apu::DigitalAudioSample Apu::compute_digital_audio_sample() const {
    return DigitalAudioSample {
        compute_ch1_digital_output(),
        compute_ch2_digital_output(),
        compute_ch3_digital_output(),
        compute_ch4_digital_output(),
    };
}

Apu::AudioSample Apu::compute_audio_sample() const {
    struct {
        int16_t ch1 {};
        int16_t ch2 {};
        int16_t ch3 {};
        int16_t ch4 {};
    } analog_output;

    // Note that DAC does its work even if channel is off.

    // TODO: consider caching channel outputs instead of recomputing them each time (shared with PCM).

    // CH1
    if (ch1.dac) {
        analog_output.ch1 = digital_to_analog_volume(compute_ch1_digital_output());
    }

    // CH2
    if (ch2.dac) {
        analog_output.ch2 = digital_to_analog_volume(compute_ch2_digital_output());
    }

    // CH3
    if (nr30.dac) {
        analog_output.ch3 = digital_to_analog_volume(compute_ch3_digital_output());
    }

    // CH4
    if (ch4.dac) {
        analog_output.ch4 = digital_to_analog_volume(compute_ch4_digital_output());
    }

    // Mix channels
    struct {
        struct ChannelStereoAnalogOutput {
            int16_t left, right;
        } ch1, ch2, ch3, ch4;
    } analog_stereo_output {};

    analog_stereo_output.ch1.left = nr51.ch1_left ? analog_output.ch1 : (int16_t)0;
    analog_stereo_output.ch2.left = nr51.ch2_left ? analog_output.ch2 : (int16_t)0;
    analog_stereo_output.ch3.left = nr51.ch3_left ? analog_output.ch3 : (int16_t)0;
    analog_stereo_output.ch4.left = nr51.ch4_left ? analog_output.ch4 : (int16_t)0;
    analog_stereo_output.ch1.right = nr51.ch1_right ? analog_output.ch1 : (int16_t)0;
    analog_stereo_output.ch2.right = nr51.ch2_right ? analog_output.ch2 : (int16_t)0;
    analog_stereo_output.ch3.right = nr51.ch3_right ? analog_output.ch3 : (int16_t)0;
    analog_stereo_output.ch4.right = nr51.ch4_right ? analog_output.ch4 : (int16_t)0;

    int32_t analog_left = (analog_stereo_output.ch1.left + analog_stereo_output.ch2.left +
                           analog_stereo_output.ch3.left + analog_stereo_output.ch4.left) /
                          4;
    int32_t analog_right = (analog_stereo_output.ch1.right + analog_stereo_output.ch2.right +
                            analog_stereo_output.ch3.right + analog_stereo_output.ch4.right) /
                           4;

    ASSERT(analog_left >= INT16_MIN && analog_left <= INT16_MAX);
    ASSERT(analog_right >= INT16_MIN && analog_right <= INT16_MAX);

    AudioSample sample {static_cast<int16_t>(analog_left), static_cast<int16_t>(analog_right)};

    // Scale by NR50 volume
    sample.left = scale_analog_volume_by_master_volume(sample.left, nr50.volume_left);
    sample.right = scale_analog_volume_by_master_volume(sample.right, nr50.volume_right);

    // Scale by software volume (physical knob)
    ASSERT(master_volume >= 0.0f && master_volume <= 1.0f);

    sample.left = static_cast<int16_t>(static_cast<float>(sample.left) * master_volume);
    sample.right = static_cast<int16_t>(static_cast<float>(sample.right) * master_volume);

    return sample;
}

void Apu::flush_pending_write() {
    if (pending_write.updater) {
        (this->*pending_write.updater)(pending_write.value);
        pending_write.updater = nullptr;
    }
}

void Apu::write_register(RegisterUpdater updater, uint8_t value) {
    pending_write.updater = updater;
    pending_write.value = value;
}

void Apu::update_nr10(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    uint8_t prev_direction = nr10.direction;
    uint8_t prev_step = nr10.step;

    nr10.pace = get_bits_range<Specs::Bits::Audio::NR10::PACE>(value);
    nr10.direction = test_bit<Specs::Bits::Audio::NR10::DIRECTION>(value);
    nr10.step = get_bits_range<Specs::Bits::Audio::NR10::STEP>(value);

    // Writing to NR10 might lead to several glitches.
    // These are the ones I discovered so far.
    if (nr52.ch1) {
        if (nr10.direction == 0) {
            // Glitch 1.
            // Updating the period sweep direction to increase mode might eventually disable the channel.
            // Usually, this happens if the period sweep mode was in decrease mode and now is in increasing mode,
            // but the actual calculation is a bit more tricky.
            // Indeed, on DMG the channel can be disabled even if the direction is not changed
            // (the channel overflows during the trigger with a period of 2048, but after a NR10 write
            // or due to a recalculation, it overflows also with 2047).
#ifdef ENABLE_CGB
            bool complement_bit = prev_direction;
#else
            // DMG glitch: the complement bit does not depend on the nr10.direction, as expected, instead it's always 1.
            bool complement_bit = true;
#endif

            const uint16_t period =
                compute_next_period_sweep_period(ch1.period_sweep.recalculation.increment, complement_bit);
            if (period >= 2048) {
                // Overflow on recalculation: disable channel.
                nr52.ch1 = false;
            }
        }
    }

    if (nr52.ch1) {
        if (ch1.period_sweep.recalculation.target_trigger_counter &&
            ch1.period_sweep.recalculation.trigger_counter < 2) {
            // Glitch 2.
            // Writing to NR10 just after it has been triggered reloads the new step as recalculation countdown.
            ch1.period_sweep.recalculation.countdown = nr10.step;
            if (!ch1.period_sweep.recalculation.countdown) {
                // If the new step is 0, the recalculation will be aborted (forever).
                ch1.period_sweep.recalculation.trigger_counter = 0;
                ch1.period_sweep.recalculation.target_trigger_counter = 0;
            }
        } else {
            // Glitch 3.
            // Under certain circumstances, if the previous step was 0 and the new one is positive,
            // the recalculation countdown might be ticked by 1.
#ifdef ENABLE_CGB
            // On CGB: the ticking happens only if the writing happens at the same time of a recalculation tick
            // (this should happen only in double speed mode). It seems that the countdown is ticked anyhow if
            // the recalculation is about to be completed (regardless the old/new step values)
            const bool tick_sweep = apu_clock_edge == ch1.period_sweep.recalculation.clock_edge &&
                                    ((ch1.period_sweep.recalculation.countdown && prev_step == 0 && nr10.step) ||
                                     ch1.period_sweep.recalculation.countdown == 1);
#else
            // On DMG: the ticking always happens (contrarily to CGB single speed mode) if the previous step was 0
            // and the new one is positive.
            const bool tick_sweep = (ch1.period_sweep.recalculation.countdown && prev_step == 0 && nr10.step);
#endif

            if (tick_sweep) {
                if (--ch1.period_sweep.recalculation.countdown == 0) {
                    period_sweep_recalculation_done();
                }
            }
        }

        // Glitch 4.
        // If the writing happens with a pace_countdown of 8 (remember that pace 0 is reloaded as 8),
        // the period sweep is ticked as if a DIV APU event happened.
        if (keep_bits<3>(ch1.period_sweep.pace_countdown) == 0) {
            period_sweep_done();
        }
    }
}

void Apu::update_nr11(uint8_t value) {
    update_nrx1(ch1, nr11, value);
}

void Apu::update_nr12(uint8_t value) {
    update_nrx2(ch1, nr52.ch1, nr12, value);
}

void Apu::update_nr13(uint8_t value) {
    if (ch1.period_sweep.just_reloaded_period) {
        // TODO: check this case.
    }

    update_nrx3(ch1, nr13, nr14, value);
}

void Apu::update_nr14(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    if (ch1.period_sweep.just_reloaded_period) {
        // Glitch.
        // If the period has just been reloaded by the period sweep, then the new period value is just ignored.
        // It also seems that the wave position is advanced in this case.
        value = discard_bits<3>(value) | nr14.period_high;
        advance_square_wave_position(ch1);
    }

    update_nrx4(ch1, nr52.ch1, nr12, nr13, nr14, value);

    if (nr14.trigger) {
        // Pace 0 is reloaded as pace 8.
        // [blargg/05-sweep-details]
        ch1.period_sweep.pace_countdown = nr10.pace ? nr10.pace : 8;

        // The shadow registers are reset.
        ch1.period_sweep.period = 0;
        ch1.period_sweep.increment = 0;
        ch1.period_sweep.recalculation.increment = 0;

        ch1.period_sweep.recalculation.from_trigger = true;

        // This countdown refers to the time the channel has been triggered (not retriggered!).
        // This is needed to emulate a few glitches occurring when the period sweep is ticked
        // nearly to the NR14 write.
        if (ch1.period_sweep.recalculation.trigger_counter == ch1.period_sweep.recalculation.target_trigger_counter) {
            ch1.period_sweep.restart_countdown = 3;
        }

        // [blargg/04-sweep]
        if (nr10.step) {
            // Trigger the channel with a step > 0 leads to a recalculation.

            // Increment is reloaded accordingly to the step.
            ch1.period_sweep.increment = concat(nr14.period_high, nr13.period_low) >> nr10.step;

            // The timing of the recalculation varies: it is bound to the NR10 step value, plus a certain delay.
            // The two delays are split into trigger_counter and countdown: this is convenient to handle the NR10
            // glitches accurately.
            if (ch1.period_sweep.recalculation.target_trigger_counter &&
                ch1.period_sweep.recalculation.trigger_counter < 2) {
                // No reload: there's a window in which the counter is not reloaded if it has been just reloaded.
            } else {
                // This seems to cover all the cases so far, but further investigations are probably needed.
                // The new target_trigger_counter varies from 2 to 4.
                // (4 is a special case that seems to happen only for step 1).
                // TODO: verify with more triple trigger cases.
                ch1.period_sweep.recalculation.target_trigger_counter =
                    2 + (ch1.period_sweep.recalculation.countdown < 2) +
                    (ch1.period_sweep.recalculation.trigger_counter == 2 &&
                     ch1.period_sweep.recalculation.countdown == nr10.step);

                // Restart the counter.
                ch1.period_sweep.recalculation.trigger_counter = 0;
            }

            // Reload countdown accordingly to the step.
            ch1.period_sweep.recalculation.countdown = nr10.step;

            // Align the recalculation with the APU clock.
            ch1.period_sweep.recalculation.clock_edge = false;
        }
    }
}

void Apu::update_nr21(uint8_t value) {
    update_nrx1(ch2, nr21, value);
}

void Apu::update_nr22(uint8_t value) {
    update_nrx2(ch2, nr52.ch2, nr22, value);
}

void Apu::update_nr23(uint8_t value) {
    update_nrx3(ch2, nr23, nr24, value);
}

void Apu::update_nr24(uint8_t value) {
    update_nrx4(ch2, nr52.ch2, nr22, nr23, nr24, value);
}

template <typename Channel, typename Nrx1>
inline void Apu::update_nrx1(Channel& ch, Nrx1& nrx1, uint8_t value) {
    uint8_t initial_length = get_bits_range<Specs::Bits::Audio::NR11::INITIAL_LENGTH_TIMER>(value);

    if (nr52.enable) {
        nrx1.duty_cycle = get_bits_range<Specs::Bits::Audio::NR11::DUTY_CYCLE>(value);
    }

#ifdef ENABLE_CGB
    if (nr52.enable) {
        nrx1.initial_length_timer = initial_length;
    }
#else
    // On DMG, length timer is loaded even though APU is disabled
    // [blargg/08-len_ctr_during_power]
    nrx1.initial_length_timer = initial_length;
#endif

    ch.length_timer = 64 - nrx1.initial_length_timer;
}

template <typename Channel, typename ChannelOnFlag, typename Nrx2>
inline void Apu::update_nrx2(Channel& ch, ChannelOnFlag& ch_on, Nrx2& nrx2, uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    uint8_t prev_volume = nrx2.initial_volume;
    bool prev_direction = nrx2.envelope_direction;
    uint8_t prev_sweep = nrx2.sweep_pace;

    nrx2.initial_volume = get_bits_range<Specs::Bits::Audio::NR12::INITIAL_VOLUME>(value);
    nrx2.envelope_direction = test_bit<Specs::Bits::Audio::NR12::ENVELOPE_DIRECTION>(value);
    nrx2.sweep_pace = get_bits_range<Specs::Bits::Audio::NR12::SWEEP_PACE>(value);

    ch.dac = nrx2.initial_volume | nrx2.envelope_direction;

    if (ch_on) {
        static constexpr auto NRX2_GLITCH_TABLE = generate_nrx2_glitch_table();

        // NRx2 glitch: volume is updated in a way that depends on the previous and current NRx2 values.
        ch.volume = NRX2_GLITCH_TABLE[prev_volume != 0][prev_sweep != 0][nrx2.sweep_pace != 0][prev_direction]
                                     [nrx2.envelope_direction][ch.volume];

        if (!ch.dac) {
            if (ch.wave.timer == 2047) {
                // If the channel is about to sample, the position of the square wave is advanced
                advance_square_wave_position(ch);
            }

            // If the DAC is turned off the channel is disabled as well
            ch_on = false;
        }
    }
}

template <typename Channel, typename Nrx3, typename Nrx4>
inline void Apu::update_nrx3(Channel& ch, Nrx3& nrx3, Nrx4& nrx4, uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nrx3.period_low = value;

    // If the channel has just sampled, timer is reloaded with the new period instantly.
    if (ch.just_sampled) {
        ch.wave.timer = concat(nrx4.period_high, nrx3.period_low);
    }
}

template <typename Channel, typename ChannelOnFlag, typename Nrx2, typename Nrx3, typename Nrx4>
inline void Apu::update_nrx4(Channel& ch, ChannelOnFlag& ch_on, Nrx2& nrx2, Nrx3& nrx3, Nrx4& nrx4, uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    bool prev_length_enable = nrx4.length_enable;

    nrx4.trigger = test_bit<Specs::Bits::Audio::NR14::TRIGGER>(value);
    nrx4.length_enable = test_bit<Specs::Bits::Audio::NR14::LENGTH_ENABLE>(value);
    nrx4.period_high = get_bits_range<Specs::Bits::Audio::NR14::PERIOD>(value);
    ASSERT(nrx4.period_high < 8);

    // Length timer clocks if length_enable has a rising edge (disabled -> enabled)
    // [blargg/03-trigger]
    if (!prev_length_enable && nrx4.length_enable) {
        if (mod<2>(div_apu) == 0) {
            tick_length_timer(ch, ch_on);
        }
    }

    // Writing with the Trigger bit set reloads the channel configuration
    if (nrx4.trigger) {
        if (!ch_on) {
            ch.trigger_delay = 3;
        } else {
            ch.trigger_delay = 2 + (ch.trigger_delay ? 1 : 0);

            if (ch.wave.timer == 2047) {
                // If the channel is about to sample, the position of the square wave is advanced
                advance_square_wave_position(ch);
            }
        }

        if (ch.dac) {
            // If the DAC is on, the channel is turned on as well
            ch_on = true;
        }

        ch.volume = nrx2.initial_volume;

        ch.wave.timer = concat(nrx4.period_high, nrx3.period_low);

        ch.volume_sweep.direction = nrx2.envelope_direction;
        ch.volume_sweep.countdown = nrx2.sweep_pace;
        ch.volume_sweep.pending_update = false;

        // If length timer is 0, it is reloaded with the maximum value (64) as well
        // [blargg/03-trigger]
        if (ch.length_timer == 0) {
            if (nrx4.length_enable && mod<2>(div_apu) == 0) {
                // Reload and clocks the length timer all at once
                ch.length_timer = 63;
            } else {
                ch.length_timer = 64;
            }
        }
    } else {
        // If the channel has just sampled, timer is reloaded with the new period instantly
        // (even if channel is not triggered).
        //  [samesuite/channel_1_freq_change_timing-cgbDE.gb]
        if (ch.just_sampled) {
            ch.wave.timer = concat(nrx4.period_high, nrx3.period_low);
        }
    }
}

void Apu::update_nr30(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr30.dac = test_bit<Specs::Bits::Audio::NR30::DAC>(value);

    if (!nr30.dac) {
        if (nr52.ch3) {
            // If channel is turned off, the channel output is updated with the high nibble of the current sampling
            // byte. It seems that if this happens while the channel is sampling, the channel loads the sampling byte
            // from wave ram depending on the current PC value.
            // TODO: does this happen on DMG too?
            if (ch3.wave.timer == 2047) {
                ch3.digital_output = get_bits_range<7, 4>(wave_ram[keep_bits<4>(program_counter)]);
            } else {
                ch3.digital_output = get_bits_range<7, 4>(ch3.sample);
            }

            // If the DAC is turned off the channel is disabled as well
            nr52.ch3 = false;
        }
    }
}

void Apu::update_nr31(uint8_t value) {
#ifdef ENABLE_CGB
    if (nr52.enable) {
        nr31.initial_length_timer = value;
    }
#else
    // On DMG, length timer is loaded even though APU is disabled
    // [blargg/08-len_ctr_during_power]
    nr31.initial_length_timer = value;
#endif

    ch3.length_timer = 256 - nr31.initial_length_timer;
}

void Apu::update_nr32(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr32.volume = get_bits_range<Specs::Bits::Audio::NR32::VOLUME>(value);
}

void Apu::update_nr33(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr33.period_low = value;
}

void Apu::update_nr34(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    bool prev_length_enable = nr34.length_enable;

    nr34.trigger = test_bit<Specs::Bits::Audio::NR34::TRIGGER>(value);
    nr34.length_enable = test_bit<Specs::Bits::Audio::NR34::LENGTH_ENABLE>(value);
    nr34.period_high = get_bits_range<Specs::Bits::Audio::NR34::PERIOD>(value);
    ASSERT(nr34.period_high < 8);

    // Length timer clocks if length_enable has a rising edge (disabled -> enabled)
    // [blargg/03-trigger]
    if (!prev_length_enable && nr34.length_enable) {
        if (mod<2>(div_apu) == 0) {
            tick_length_timer(ch3, nr52.ch3);
        }
    }

    // Writing with the Trigger bit set reloads the channel configurations
    if (nr34.trigger) {
        ch3.trigger_delay = 3;

        if (nr52.ch3) {
            if (ch3.wave.timer == 2047) {
                // If the channel is about to sample, the first byte of wave ram is used for the sampling instead.
                ch3.sample = wave_ram[0];
#ifndef ENABLE_CGB
                // On DMG, re-triggering CH3 while it is reading wave ram corrupts wave ram.
                // * If the buffer position read from wave ram is [0:3], then only the first byte
                //   of wave ram is corrupted with the byte that is read at that moment.
                // * If the buffer position read from wave ram is [4:15], then the first 4 bytes
                //   of wave ram are corrupted with the (aligned) 4 bytes that are read at the moment
                // [blargg/10-wave_trigger_while_on]

                uint8_t next_wave_position_byte =
                    ch3.wave.position.low_nibble ? mod<16>(ch3.wave.position.byte + 1) : ch3.wave.position.byte;

                if (next_wave_position_byte < 4) {
                    wave_ram[0] = static_cast<uint8_t>(wave_ram[next_wave_position_byte]);
                } else {
                    const uint8_t base = next_wave_position_byte / 4;
                    wave_ram[0] = static_cast<uint8_t>(wave_ram[4 * base]);
                    wave_ram[1] = static_cast<uint8_t>(wave_ram[4 * base + 1]);
                    wave_ram[2] = static_cast<uint8_t>(wave_ram[4 * base + 2]);
                    wave_ram[3] = static_cast<uint8_t>(wave_ram[4 * base + 3]);
                }
#endif
            }
            // The output of the channel is updated with the first nibble of the sample byte.
            ch3.digital_output = get_bits_range<7, 4>(ch3.sample);
        }

        // The wave position is reset instantly.
        ch3.wave.position.low_nibble = false;
        ch3.wave.position.byte = 0;

        // The timer is reloaded.
        ch3.wave.timer = concat(nr34.period_high, nr33.period_low);

        if (nr30.dac) {
            // If the DAC is on, the channel is turned on as well
            nr52.ch3 = true;
        }

        // If length timer is 0, it is reloaded with the maximum value (256) as well
        // [blargg/03-trigger]
        if (ch3.length_timer == 0) {
            if (nr34.length_enable && mod<2>(div_apu) == 0) {
                // Reload and clocks the length timer all at once
                ch3.length_timer = 255;
            } else {
                ch3.length_timer = 256;
            }
        }
    }
}

void Apu::update_nr41(uint8_t value) {
    uint8_t initial_length = get_bits_range<Specs::Bits::Audio::NR41::INITIAL_LENGTH_TIMER>(value);

#ifdef ENABLE_CGB
    if (nr52.enable) {
        nr41.initial_length_timer = initial_length;
    }
#else
    // On DMG, length timer is loaded even though APU is disabled
    // [blargg/08-len_ctr_during_power]
    nr41.initial_length_timer = initial_length;
#endif

    ch4.length_timer = 64 - nr41.initial_length_timer;
}

void Apu::update_nr42(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr42.initial_volume = get_bits_range<Specs::Bits::Audio::NR42::INITIAL_VOLUME>(value);
    nr42.envelope_direction = test_bit<Specs::Bits::Audio::NR42::ENVELOPE_DIRECTION>(value);
    nr42.sweep_pace = get_bits_range<Specs::Bits::Audio::NR42::SWEEP_PACE>(value);

    ch4.dac = nr42.initial_volume | nr42.envelope_direction;
    if (!ch4.dac) {
        // If the DAC is turned off the channel is disabled as well
        nr52.ch4 = false;
    }
}

void Apu::update_nr43(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr43.clock_shift = get_bits_range<Specs::Bits::Audio::NR43::CLOCK_SHIFT>(value);
    nr43.lfsr_width = test_bit<Specs::Bits::Audio::NR43::LFSR_WIDTH>(value);
    nr43.clock_divider = get_bits_range<Specs::Bits::Audio::NR43::CLOCK_DIVIDER>(value);
}

void Apu::update_nr44(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    bool prev_length_enable = nr44.length_enable;

    nr44.trigger = test_bit<Specs::Bits::Audio::NR44::TRIGGER>(value);
    nr44.length_enable = test_bit<Specs::Bits::Audio::NR44::LENGTH_ENABLE>(value);

    // Length timer clocks if length_enable has a rising edge (disabled -> enabled)
    // [blargg/03-trigger]
    if (!prev_length_enable && nr44.length_enable) {
        if (mod<2>(div_apu) == 0) {
            tick_length_timer(ch4, nr52.ch4);
        }
    }

    // Writing with the Trigger bit set reloads the channel configurations
    if (nr44.trigger) {
        if (ch4.dac) {
            // If the DAC is on, the channel is turned on as well
            nr52.ch4 = true;
        }

        ch4.volume = nr42.initial_volume;

        ch4.volume_sweep.direction = nr42.envelope_direction;
        ch4.volume_sweep.countdown = nr42.sweep_pace;
        ch4.volume_sweep.pending_update = false;

        ch4.lfsr = 0;

        // If length timer is 0, it is reloaded with the maximum value (64) as well
        // [blargg/03-trigger]
        if (ch4.length_timer == 0) {
            if (nr44.length_enable && mod<2>(div_apu) == 0) {
                // Reload and clocks the length timer all at once
                ch4.length_timer = 63;
            } else {
                ch4.length_timer = 64;
            }
        }
    }
}

void Apu::update_nr50(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr50.vin_left = test_bit<Specs::Bits::Audio::NR50::VIN_LEFT>(value);
    nr50.volume_left = get_bits_range<Specs::Bits::Audio::NR50::VOLUME_LEFT>(value);
    nr50.vin_right = test_bit<Specs::Bits::Audio::NR50::VIN_RIGHT>(value);
    nr50.volume_right = get_bits_range<Specs::Bits::Audio::NR50::VOLUME_RIGHT>(value);
}

void Apu::update_nr51(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr51.ch4_left = test_bit<Specs::Bits::Audio::NR51::CH4_LEFT>(value);
    nr51.ch3_left = test_bit<Specs::Bits::Audio::NR51::CH3_LEFT>(value);
    nr51.ch2_left = test_bit<Specs::Bits::Audio::NR51::CH2_LEFT>(value);
    nr51.ch1_left = test_bit<Specs::Bits::Audio::NR51::CH1_LEFT>(value);
    nr51.ch4_right = test_bit<Specs::Bits::Audio::NR51::CH4_RIGHT>(value);
    nr51.ch3_right = test_bit<Specs::Bits::Audio::NR51::CH3_RIGHT>(value);
    nr51.ch2_right = test_bit<Specs::Bits::Audio::NR51::CH2_RIGHT>(value);
    nr51.ch1_right = test_bit<Specs::Bits::Audio::NR51::CH1_RIGHT>(value);
}

void Apu::update_nr52(uint8_t value) {
    const bool en = test_bit<Specs::Bits::Audio::NR52::AUDIO_ENABLE>(value);
    if (en != nr52.enable) {
        en ? turn_on() : turn_off();
        nr52.enable = en;
    }
}

template <uint16_t address>
void Apu::update_wave_ram(uint8_t value) {
    if (!nr52.ch3) {
        // Wave ram is accessed normally if CH3 is off.
        wave_ram[address - Specs::Registers::Sound::WAVE0] = value;
        return;
    }

    // Writing to wave ram corrupts the byte the channel is currently reading.
    // On CGB, this always happens, while on DMG it happens only when write
    // happens at the same the channel is sampling.
#ifdef ENABLE_CGB
    // [blargg/12-wave]
    wave_ram[ch3.wave.position.byte] = value;
#else
    // [blargg/12-wave_write_while_on]
    if (ch3.just_sampled) {
        wave_ram[ch3.wave.position.byte] = value;
    }
#endif
}