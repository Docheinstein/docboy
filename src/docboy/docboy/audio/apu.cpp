#include "docboy/audio/apu.h"

#include "docboy/bootrom/helpers.h"
#include "docboy/timers/timers.h"

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

inline uint8_t operator<<(uint8_t v, BitRange r) {
    return v << r.lsb;
}

template <typename Channel, typename ChannelOnFlag>
inline void tick_channel_length_timer(Channel& ch, ChannelOnFlag& ch_on) {
    if (ch.length_timer > 0) {
        if (--ch.length_timer == 0) {
            // Length timer expired: turn off the channel
            ch_on = false;
        }
    }
}

template <typename Channel, typename ChannelOnFlag, typename Nrx4>
inline void tick_channel_length_timer(Channel& ch, ChannelOnFlag& ch_on, const Nrx4& nrx4) {
    if (nrx4.length_enable) {
        tick_channel_length_timer(ch, ch_on);
    }
}

template <typename Channel, typename ChannelOnFlag>
inline void tick_channel_volume_sweep(Channel& ch, const ChannelOnFlag& ch_on) {
    if (ch_on && ch.volume_sweep.pace /* 0 disables volume sweep */) {
        if (++ch.volume_sweep.timer >= ch.volume_sweep.pace) {
            // Volume sweep expired
            ch.volume_sweep.expired = true;
            ch.volume_sweep.timer = 0;
        }
    }
}

template <typename Channel>
inline void handle_channel_volume_sweep_expired(Channel& ch) {
    if (ch.volume_sweep.expired) {
        ch.volume_sweep.expired = false;

        // Increase/decrease volume accordingly to sweep direction (if possible).
        if (ch.volume_sweep.direction) {
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

template <typename Channel, typename ChannelOnFlag, typename Nrx3, typename Nrx4>
inline void tick_square_wave_channel(Channel& ch, const ChannelOnFlag& ch_on, const Nrx3& nrx3, const Nrx4& nrx4) {
    if (ch_on) {
        // Wave position does not change immediately after channel's trigger.
        if (!ch.trigger_delay) {
            if (++ch.wave.timer == 2048) {
                // Advance square wave position
                ch.wave.position = mod<8>(ch.wave.position + 1);

                // Reload period timer
                ch.wave.timer = nrx4.period_high << 8 | nrx3.period_low;
                ASSERT(ch.wave.timer < 2048);
            }
        } else {
            --ch.trigger_delay;
        }
    }
}
} // namespace

Apu::Apu(Timers& timers) :
    timers {timers} {
    reset();
}

void Apu::set_volume(float volume) {
    ASSERT(volume >= 0.0f && volume <= 1.0f);
    master_volume = volume;
}

void Apu::set_sample_rate(double rate) {
    sample_period = (double)Specs::Frequencies::CLOCK / rate;
}

void Apu::set_audio_sample_callback(std::function<void(const AudioSample)>&& callback) {
    audio_sample_callback = std::move(callback);
}

void Apu::reset() {
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
        wave_ram[i] = 0;
    }

    ticks = 0;

    // Length timers are not reset
    // [blargg/08-len_ctr_during_power

    ch1.dac = true;
    ch1.volume = 0;
    ch1.wave.position = 0;
    ch1.wave.timer = nr14.period_high << 8 | nr13.period_low;
    ch1.volume_sweep.direction = false;
    ch1.volume_sweep.pace = 0;
    ch1.volume_sweep.timer = 0;
    ch1.volume_sweep.expired = false;
    ch1.period_sweep.enabled = false;
    ch1.period_sweep.period = 0;
    ch1.period_sweep.timer = 0;

    ch2.dac = false;
    ch2.volume = 0;
    ch2.wave.position = 0;
    ch2.wave.timer = nr24.period_high << 8 | nr23.period_low;
    ch2.volume_sweep.direction = false;
    ch2.volume_sweep.pace = 0;
    ch2.volume_sweep.timer = 0;
    ch2.volume_sweep.expired = false;

    ch3.wave.position = 0;
    ch3.wave.timer = 0;

    ch4.dac = false;
    ch4.volume = 0;
    ch4.wave.timer = 0;
    ch4.volume_sweep.direction = false;
    ch4.volume_sweep.pace = 0;
    ch4.volume_sweep.timer = 0;
    ch4.volume_sweep.expired = false;
    ch4.lfsr = 0;

    prev_div_bit_4 = false;
    div_apu = 2;
}

inline uint8_t Apu::compute_ch1_digital_output() const {
    uint8_t digital_output {};

    if (nr52.ch1) {
        ASSERT(ch1.dac);
        ASSERT(nr11.duty_cycle < 4);
        ASSERT(ch1.wave.position < 8);
        ASSERT(ch1.volume <= 0xF);

        const bool digital_amplitude = SQUARE_WAVES[nr11.duty_cycle][ch1.wave.position];
        digital_output = digital_amplitude * ch1.volume;

        ASSERT(digital_output <= 0xF);
    }

    return digital_output;
}

inline uint8_t Apu::compute_ch2_digital_output() const {
    uint8_t digital_output {};

    if (nr52.ch2) {
        ASSERT(ch2.dac);
        ASSERT(nr21.duty_cycle < 4);
        ASSERT(ch2.wave.position < 8);
        ASSERT(ch2.volume <= 0xF);

        const bool digital_amplitude = SQUARE_WAVES[nr21.duty_cycle][ch2.wave.position];
        digital_output = digital_amplitude * ch2.volume;

        ASSERT(digital_output <= 0xF);
    }

    return digital_output;
}

inline uint8_t Apu::compute_ch3_digital_output() const {
    uint8_t digital_output {};

    if (nr52.ch3 && nr32.volume) {
        ASSERT(nr30.dac);
        ASSERT(ch3.wave.play_position < decltype(wave_ram)::Size);
        ASSERT(nr32.volume <= 0b11);

        if (mod<2>(ch3.wave.play_position)) {
            // Low nibble
            digital_output = keep_bits<4>(wave_ram[ch3.wave.play_position]);
        } else {
            // High nibble
            digital_output = wave_ram[ch3.wave.play_position] >> 4;
        }

        // Scale by volume
        digital_output = digital_output >> (nr32.volume - 1);

        ASSERT(digital_output <= 0xF);
    }

    return digital_output;
}

inline uint8_t Apu::compute_ch4_digital_output() const {
    uint8_t digital_output {};

    if (nr52.ch4) {
        ASSERT(ch4.dac);
        ASSERT(ch4.volume <= 0xF);

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

inline uint32_t Apu::compute_ch1_next_period_sweep_period() {
    // P_t+1 = P_t ± P_t / (2^step)
    int32_t period = ch1.period_sweep.period + (nr10.direction ? -1 : 1) * (ch1.period_sweep.period >> nr10.step);

    // Store whether period sweep is in decreasing mode after a recalculation
    if (!ch1.period_sweep.decreasing) {
        ch1.period_sweep.decreasing = nr10.direction == 1;
    }

    ASSERT(period >= 0);
    return period;
}

inline void Apu::tick_ch1_period_sweep() {
    // Update period sweep (CH1 only)
    if (nr52.ch1 && ch1.period_sweep.enabled) {
        if (++ch1.period_sweep.timer >= ch1.period_sweep.pace) {
            if (nr10.pace) {
                // Compute new period
                uint32_t new_period = compute_ch1_next_period_sweep_period();

                if (new_period >= 2048) {
                    // Period overflow: turn off the channel
                    nr52.ch1 = false;
                }

                if (nr10.step) {
                    // Write back period to the internal period register and to NR13/NR14
                    ch1.period_sweep.period = new_period;

                    nr14.period_high = get_bits_range<11, 8>(new_period);
                    nr13.period_low = keep_bits<8>(new_period);

                    // Period calculation is performed a second time for overflow check
                    // (but result is not written back)
                    // [blargg/04-sweep]
                    if (compute_ch1_next_period_sweep_period() >= 2048) {
                        nr52.ch1 = false;
                    }
                }
            }

            // Pace 0 is reloaded as pace 8
            // [blargg/05-sweep-details]
            ch1.period_sweep.pace = nr10.pace > 0 ? nr10.pace : 8;
            ch1.period_sweep.timer = 0;
        }
    }
}

void Apu::tick_ch4() {
    if (nr52.ch4) {
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

void Apu::tick_ch3() {
    if (nr52.ch3) {
        // It seems that trigger takes 5 ticks to have effect
        // TODO: verify
        if (ch3.trigger_delay) {
            ch3.trigger_delay--;

            if (ch3.trigger_delay == 0) {
                ch3.retrigger = false;

                ch3.wave.timer = nr34.period_high << 8 | nr33.period_low;
                ch3.wave.position = 0;
            }
        }

        if (++ch3.wave.timer == 2048) {
            // Advance square wave position
            ch3.wave.position = mod<32>(ch3.wave.position + 1);

            ch3.wave.play_position = ch3.wave.position >> 1;

            // Reload period timer
            ch3.wave.timer = nr34.period_high << 8 | nr33.period_low;

            ch3.last_read_tick = ticks;

            ASSERT(ch3.wave.timer < 2048);
        }

        // Re-triggering CH3 while it is reading wave ram corrupts wave ram.
        // * If the buffer position read from wave ram is [0:3], then only the first byte
        //   of wave ram is corrupted with the byte that is read at that moment.
        // * If the buffer position read from wave ram is [4:15], then the first 4 bytes
        //   of wave ram are corrupted with the (aligned) 4 bytes that are read at the moment
        // [blargg/10-wave_trigger_while_on]
        if (ch3.retrigger && ch3.trigger_delay == 3) {
            if (ticks == ch3.last_read_tick) {
                if (ch3.wave.play_position < 4) {
                    wave_ram[0] = static_cast<uint8_t>(wave_ram[ch3.wave.play_position]);
                } else {
                    const uint8_t base = ch3.wave.play_position / 4;
                    wave_ram[0] = static_cast<uint8_t>(wave_ram[4 * base]);
                    wave_ram[1] = static_cast<uint8_t>(wave_ram[4 * base + 1]);
                    wave_ram[2] = static_cast<uint8_t>(wave_ram[4 * base + 2]);
                    wave_ram[3] = static_cast<uint8_t>(wave_ram[4 * base + 3]);
                }
            }
        }

        // Writing to wave ram while CH3 is reading a byte corrupts it:
        // the byte of wave ram that is currently read is wrote instead.
        // [blargg/12-wave_write_while_on]
        // TODO: check exact timing (I guess this should take place in write_wave_ram instead)
        if (ch3.pending_wave_write.pending) {
            ch3.pending_wave_write.pending = false;

            if (ticks == ch3.last_read_tick) {
                wave_ram[ch3.wave.play_position] = ch3.pending_wave_write.value;
            }
        }
    }
}

void Apu::tick_t0() {
    // Do nothing if APU is off (but advance the sampler anyhow).
    if (!nr52.enable) {
        tick_sampler();
        return;
    }

    ASSERT(!(nr52.ch1 && !ch1.dac));
    ASSERT(!(nr52.ch2 && !ch2.dac));
    ASSERT(!(nr52.ch3 && !nr30.dac));
    ASSERT(!(nr52.ch4 && !ch4.dac));

    // Increase DIV-APU each time DIV[4] has a falling edge (~512Hz)
    const bool div_bit_4 = test_bit<4>(timers.read_div());
    if (prev_div_bit_4 && !div_bit_4) {
        div_apu++;

        // Eventually update channel's volume, if volume sweep is expired.
        // This doesn't happen at the same time the volume sweep overflows,
        // but only the next DIV_APU event.
        // [samesuite/div_trigger_volume_10,
        //  samesuite/div_write_trigger_volume,
        //  samesuite/div_write_trigger_volume_10]
        handle_channel_volume_sweep_expired(ch1);
        handle_channel_volume_sweep_expired(ch2);
        handle_channel_volume_sweep_expired(ch4);

        if (mod<2>(div_apu) == 0) {
            // Update length timer
            tick_channel_length_timer(ch1, nr52.ch1, nr14);
            tick_channel_length_timer(ch2, nr52.ch2, nr24);
            tick_channel_length_timer(ch3, nr52.ch3, nr34);
            tick_channel_length_timer(ch4, nr52.ch4, nr44);

            if (mod<4>(div_apu) == 0) {
                // Update period sweep (CH1 only)
                tick_ch1_period_sweep();

                if (mod<8>(div_apu) == 0) {
                    // Update volume sweep
                    tick_channel_volume_sweep(ch1, nr52.ch1);
                    tick_channel_volume_sweep(ch2, nr52.ch2);
                    tick_channel_volume_sweep(ch4, nr52.ch4);
                }
            }
        }
    }

    prev_div_bit_4 = div_bit_4;

    // Update wave timers.
    tick_square_wave_channel(ch1, nr52.ch1, nr13, nr14);
    tick_square_wave_channel(ch2, nr52.ch2, nr23, nr24);
    tick_ch3();
    tick_ch4();

    tick_sampler();

#ifdef ENABLE_AUDIO_PCM
    update_pcm();
#endif
}

void Apu::tick_t1() {
    tick_sampler();
}

void Apu::tick_t2() {
    if (nr52.ch3) {
        tick_ch3();
    }

    tick_sampler();

#ifdef ENABLE_AUDIO_PCM
    update_pcm();
#endif
}

void Apu::tick_t3() {
    tick_sampler();
}

void Apu::tick_sampler() {
    ASSERT(sample_period > 0.0);

    if (++ticks >= static_cast<uint64_t>(next_tick_sample)) {
        next_tick_sample += sample_period;

        if (audio_sample_callback) {
            audio_sample_callback(compute_audio_sample());
        }
    }
}

void Apu::turn_on() {
    prev_div_bit_4 = true;
    div_apu = 0;
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
    ch1.wave.position = 0;
    ch1.volume_sweep.expired = false;

    ch2.dac = false;
    ch2.wave.position = 0;
    ch2.volume_sweep.expired = false;

    ch3.wave.position = 0;

    ch4.dac = false;
    ch4.volume_sweep.expired = false;
}

void Apu::save_state(Parcel& parcel) const {
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

    parcel.write_uint64(ticks);

    parcel.write_bool(ch1.dac);
    parcel.write_uint8(ch1.volume);
    parcel.write_uint8(ch1.length_timer);
    parcel.write_uint8(ch1.trigger_delay);
    parcel.write_uint8(ch1.wave.position);
    parcel.write_uint16(ch1.wave.timer);
    parcel.write_bool(ch1.volume_sweep.direction);
    parcel.write_uint8(ch1.volume_sweep.pace);
    parcel.write_uint8(ch1.volume_sweep.timer);
    parcel.write_bool(ch1.volume_sweep.expired);
    parcel.write_bool(ch1.period_sweep.enabled);
    parcel.write_uint16(ch1.period_sweep.period);
    parcel.write_uint8(ch1.period_sweep.pace);
    parcel.write_uint8(ch1.period_sweep.timer);
    parcel.write_bool(ch1.period_sweep.decreasing);

    parcel.write_bool(ch2.dac);
    parcel.write_uint8(ch2.volume);
    parcel.write_uint8(ch2.length_timer);
    parcel.write_uint8(ch2.trigger_delay);
    parcel.write_uint8(ch2.wave.position);
    parcel.write_uint16(ch2.wave.timer);
    parcel.write_bool(ch2.volume_sweep.direction);
    parcel.write_uint8(ch2.volume_sweep.pace);
    parcel.write_uint8(ch2.volume_sweep.timer);
    parcel.write_bool(ch2.volume_sweep.expired);

    parcel.write_uint16(ch3.length_timer);
    parcel.write_uint8(ch3.wave.position);
    parcel.write_uint16(ch3.wave.timer);
    parcel.write_uint8(ch3.wave.play_position);
    parcel.write_bool(ch3.retrigger);
    parcel.write_uint8(ch3.trigger_delay);
    parcel.write_uint64(ch3.last_read_tick);
    parcel.write_bool(ch3.pending_wave_write.pending);
    parcel.write_uint8(ch3.pending_wave_write.value);

    parcel.write_bool(ch4.dac);
    parcel.write_uint8(ch4.volume);
    parcel.write_uint8(ch4.length_timer);
    parcel.write_uint16(ch4.wave.timer);
    parcel.write_bool(ch4.volume_sweep.direction);
    parcel.write_uint8(ch4.volume_sweep.pace);
    parcel.write_uint8(ch4.volume_sweep.timer);
    parcel.write_bool(ch4.volume_sweep.expired);
    parcel.write_uint16(ch4.lfsr);
}

void Apu::load_state(Parcel& parcel) {
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

    ticks = parcel.read_uint64();
    next_tick_sample = static_cast<double>(ticks) + sample_period;

    ch1.dac = parcel.read_bool();
    ch1.volume = parcel.read_uint8();
    ch1.length_timer = parcel.read_uint8();
    ch1.trigger_delay = parcel.read_uint8();
    ch1.wave.position = parcel.read_uint8();
    ch1.wave.timer = parcel.read_uint16();
    ch1.volume_sweep.direction = parcel.read_bool();
    ch1.volume_sweep.pace = parcel.read_uint8();
    ch1.volume_sweep.timer = parcel.read_uint8();
    ch1.volume_sweep.expired = parcel.read_bool();
    ch1.period_sweep.enabled = parcel.read_bool();
    ch1.period_sweep.period = parcel.read_uint16();
    ch1.period_sweep.pace = parcel.read_uint8();
    ch1.period_sweep.timer = parcel.read_uint8();
    ch1.period_sweep.decreasing = parcel.read_bool();

    ch2.dac = parcel.read_bool();
    ch2.volume = parcel.read_uint8();
    ch2.length_timer = parcel.read_uint8();
    ch2.trigger_delay = parcel.read_uint8();
    ch2.wave.position = parcel.read_uint8();
    ch2.wave.timer = parcel.read_uint16();
    ch2.volume_sweep.direction = parcel.read_bool();
    ch2.volume_sweep.pace = parcel.read_uint8();
    ch2.volume_sweep.timer = parcel.read_uint8();
    ch2.volume_sweep.expired = parcel.read_bool();

    ch3.length_timer = parcel.read_uint16();
    ch3.wave.position = parcel.read_uint8();
    ch3.wave.timer = parcel.read_uint16();
    ch3.wave.play_position = parcel.read_uint8();
    ch3.retrigger = parcel.read_bool();
    ch3.trigger_delay = parcel.read_uint8();
    ch3.last_read_tick = parcel.read_uint64();
    ch3.pending_wave_write.pending = parcel.read_bool();
    ch3.pending_wave_write.value = parcel.read_uint8();

    ch4.dac = parcel.read_bool();
    ch4.volume = parcel.read_uint8();
    ch4.length_timer = parcel.read_uint8();
    ch4.wave.timer = parcel.read_uint16();
    ch4.volume_sweep.direction = parcel.read_bool();
    ch4.volume_sweep.pace = parcel.read_uint8();
    ch4.volume_sweep.timer = parcel.read_uint8();
    ch4.volume_sweep.expired = parcel.read_bool();
    ch4.lfsr = parcel.read_uint16();
}

uint8_t Apu::read_nr10() const {
    return 0b10000000 | nr10.pace << Specs::Bits::Audio::NR10::PACE |
           nr10.direction << Specs::Bits::Audio::NR10::DIRECTION | nr10.step << Specs::Bits::Audio::NR10::STEP;
}

void Apu::write_nr10(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr10.pace = get_bits_range<Specs::Bits::Audio::NR10::PACE>(value);
    nr10.direction = test_bit<Specs::Bits::Audio::NR10::DIRECTION>(value);
    nr10.step = get_bits_range<Specs::Bits::Audio::NR10::STEP>(value);

    // After the period is recalculated with decreasing mode, if the sweep period mode
    // is changed again to increasing mode, the channel is disabled instantly.
    // [blargg/05-sweep-details]
    if (ch1.period_sweep.decreasing && nr10.direction == 0) {
        nr52.ch1 = false;
    }
}

uint8_t Apu::read_nr11() const {
    return 0b00111111 | nr11.duty_cycle << Specs::Bits::Audio::NR11::DUTY_CYCLE;
}

void Apu::write_nr11(uint8_t value) {
    uint8_t initial_length = get_bits_range<Specs::Bits::Audio::NR11::INITIAL_LENGTH_TIMER>(value);

    if (nr52.enable) {
        nr11.duty_cycle = get_bits_range<Specs::Bits::Audio::NR11::DUTY_CYCLE>(value);
        nr11.initial_length_timer = initial_length;
    }

    // It seems that length timer is loaded even though APU is disabled
    // [blargg/08-len_ctr_during_power]
    ch1.length_timer = 64 - initial_length;
}

uint8_t Apu::read_nr12() const {
    return nr12.initial_volume << Specs::Bits::Audio::NR12::INITIAL_VOLUME |
           nr12.envelope_direction << Specs::Bits::Audio::NR12::ENVELOPE_DIRECTION |
           nr12.sweep_pace << Specs::Bits::Audio::NR12::SWEEP_PACE;
}

void Apu::write_nr12(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr12.initial_volume = get_bits_range<Specs::Bits::Audio::NR12::INITIAL_VOLUME>(value);
    nr12.envelope_direction = test_bit<Specs::Bits::Audio::NR12::ENVELOPE_DIRECTION>(value);
    nr12.sweep_pace = get_bits_range<Specs::Bits::Audio::NR12::SWEEP_PACE>(value);

    ch1.dac = nr12.initial_volume | nr12.envelope_direction;
    if (!ch1.dac) {
        // If the DAC is turned off the channel is disabled as well
        nr52.ch1 = false;
    }
}

uint8_t Apu::read_nr13() const {
    return 0xFF;
}

void Apu::write_nr13(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr13.period_low = value;
}

uint8_t Apu::read_nr14() const {
    return 0b10111111 | nr14.length_enable << Specs::Bits::Audio::NR14::LENGTH_ENABLE;
}

void Apu::write_nr14(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    bool prev_length_enable = nr14.length_enable;

    nr14.trigger = test_bit<Specs::Bits::Audio::NR14::TRIGGER>(value);
    nr14.length_enable = test_bit<Specs::Bits::Audio::NR14::LENGTH_ENABLE>(value);
    nr14.period_high = get_bits_range<Specs::Bits::Audio::NR14::PERIOD>(value);
    ASSERT(nr14.period_high < 8);

    // Length timer clocks if length_enable has a rising edge (disabled -> enabled)
    // [blargg/03-trigger]
    if (!prev_length_enable && nr14.length_enable) {
        if (mod<2>(div_apu) == 0) {
            tick_channel_length_timer(ch1, nr52.ch1);
        }
    }

    // Writing with the Trigger bit set reloads the channel configuration
    if (nr14.trigger) {
        ch1.trigger_delay = !nr52.ch1 ? 3 : 2;

        if (ch1.dac) {
            // If the DAC is on, the channel is turned on as well
            nr52.ch1 = true;
        }

        ch1.volume = nr12.initial_volume;

        ch1.wave.timer = nr14.period_high << 8 | nr13.period_low;

        ch1.volume_sweep.direction = nr12.envelope_direction;
        ch1.volume_sweep.pace = nr12.sweep_pace;
        ch1.volume_sweep.timer = 0;
        ch1.volume_sweep.expired = false;

        ch1.period_sweep.enabled = nr10.pace || nr10.step;
        ch1.period_sweep.period = nr14.period_high << 8 | (nr13.period_low);

        // Pace 0 is reloaded as pace 8.
        // [blargg/05-sweep-details]
        ch1.period_sweep.pace = nr10.pace > 0 ? nr10.pace : 8;

        ch1.period_sweep.timer = 0;
        ch1.period_sweep.decreasing = false;

        // Period sweep is updated on trigger
        // [blargg/04-sweep]
        if (nr10.step) {
            uint32_t new_period = compute_ch1_next_period_sweep_period();

            if (new_period >= 2048) {
                nr52.ch1 = false;
            }
        }

        // If length timer is 0, it is reloaded with the maximum value (64) as well
        // [blargg/03-trigger]
        if (ch1.length_timer == 0) {
            if (nr14.length_enable && mod<2>(div_apu) == 0) {
                // Reload and clocks the length timer all at once
                ch1.length_timer = 63;
            } else {
                ch1.length_timer = 64;
            }
        }
    }
}

uint8_t Apu::read_nr21() const {
    return 0b00111111 | nr21.duty_cycle << Specs::Bits::Audio::NR21::DUTY_CYCLE;
}

void Apu::write_nr21(uint8_t value) {
    uint8_t initial_length = get_bits_range<Specs::Bits::Audio::NR21::INITIAL_LENGTH_TIMER>(value);

    if (nr52.enable) {
        nr21.duty_cycle = get_bits_range<Specs::Bits::Audio::NR21::DUTY_CYCLE>(value);
        nr21.initial_length_timer = initial_length;
    }

    // It seems that length timer is loaded even though APU is disabled
    // [blargg/08-len_ctr_during_power]
    ch2.length_timer = 64 - initial_length;
}

uint8_t Apu::read_nr22() const {
    return nr22.initial_volume << Specs::Bits::Audio::NR22::INITIAL_VOLUME |
           nr22.envelope_direction << Specs::Bits::Audio::NR22::ENVELOPE_DIRECTION |
           nr22.sweep_pace << Specs::Bits::Audio::NR22::SWEEP_PACE;
}

void Apu::write_nr22(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr22.initial_volume = get_bits_range<Specs::Bits::Audio::NR22::INITIAL_VOLUME>(value);
    nr22.envelope_direction = test_bit<Specs::Bits::Audio::NR22::ENVELOPE_DIRECTION>(value);
    nr22.sweep_pace = get_bits_range<Specs::Bits::Audio::NR22::SWEEP_PACE>(value);

    ch2.dac = nr22.initial_volume | nr22.envelope_direction;
    if (!ch2.dac) {
        // If the DAC is turned off the channel is disabled as well
        nr52.ch2 = false;
    }
}

uint8_t Apu::read_nr23() const {
    return 0xFF;
}

void Apu::write_nr23(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr23.period_low = value;
}

uint8_t Apu::read_nr24() const {
    return 0b10111111 | nr24.length_enable << Specs::Bits::Audio::NR24::LENGTH_ENABLE;
}

void Apu::write_nr24(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    bool prev_length_enable = nr24.length_enable;

    nr24.trigger = test_bit<Specs::Bits::Audio::NR24::TRIGGER>(value);
    nr24.length_enable = test_bit<Specs::Bits::Audio::NR24::LENGTH_ENABLE>(value);
    nr24.period_high = get_bits_range<Specs::Bits::Audio::NR24::PERIOD>(value);
    ASSERT(nr24.period_high < 8);

    // Length timer clocks if length_enable has a rising edge (disabled -> enabled)
    // [blargg/03-trigger]
    if (!prev_length_enable && nr24.length_enable) {
        if (mod<2>(div_apu) == 0) {
            tick_channel_length_timer(ch2, nr52.ch2);
        }
    }

    // Writing with the Trigger bit set reloads the channel configuration
    if (nr24.trigger) {
        ch2.trigger_delay = !nr52.ch2 ? 3 : 2;

        if (ch2.dac) {
            // If the DAC is on, the channel is turned on as well
            nr52.ch2 = true;
        }

        ch2.volume = nr22.initial_volume;

        ch2.wave.timer = nr24.period_high << 8 | nr23.period_low;

        ch2.volume_sweep.direction = nr22.envelope_direction;
        ch2.volume_sweep.pace = nr22.sweep_pace;
        ch2.volume_sweep.timer = 0;
        ch2.volume_sweep.expired = false;

        // If length timer is 0, it is reloaded with the maximum value (64) as well
        // [blargg/03-trigger]
        if (ch2.length_timer == 0) {
            if (nr24.length_enable && mod<2>(div_apu) == 0) {
                // Reload and clocks the length timer all at once
                ch2.length_timer = 63;
            } else {
                ch2.length_timer = 64;
            }
        }
    }
}

uint8_t Apu::read_nr30() const {
    return 0b01111111 | nr30.dac << Specs::Bits::Audio::NR30::DAC;
}

void Apu::write_nr30(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr30.dac = test_bit<Specs::Bits::Audio::NR30::DAC>(value);

    if (!nr30.dac) {
        // If the DAC is turned off the channel is disabled as well
        nr52.ch3 = false;
    }
}

uint8_t Apu::read_nr31() const {
    return 0xFF;
}

void Apu::write_nr31(uint8_t value) {
    if (nr52.enable) {
        nr31.initial_length_timer = value;
    }

    // It seems that length timer is loaded even though APU is disabled
    // [blargg/08-len_ctr_during_power]
    ch3.length_timer = 256 - value;
}

uint8_t Apu::read_nr32() const {
    return 0b10011111 | nr32.volume << Specs::Bits::Audio::NR32::VOLUME;
}

void Apu::write_nr32(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr32.volume = get_bits_range<Specs::Bits::Audio::NR32::VOLUME>(value);
}

uint8_t Apu::read_nr33() const {
    return 0xFF;
}

void Apu::write_nr33(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr33.period_low = value;
}

uint8_t Apu::read_nr34() const {
    return 0b10111111 | nr34.length_enable << Specs::Bits::Audio::NR34::LENGTH_ENABLE;
}

void Apu::write_nr34(uint8_t value) {
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
            tick_channel_length_timer(ch3, nr52.ch3);
        }
    }

    // Writing with the Trigger bit set reloads the channel configurations
    if (nr34.trigger) {
        ch3.retrigger = nr52.ch3;
        ch3.trigger_delay = 5;

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

uint8_t Apu::read_nr41() const {
    return 0xFF;
}

void Apu::write_nr41(uint8_t value) {
    uint8_t initial_length = get_bits_range<Specs::Bits::Audio::NR41::INITIAL_LENGTH_TIMER>(value);

    if (nr52.enable) {
        nr41.initial_length_timer = initial_length;
    }

    // It seems that length timer is loaded even though APU is disabled
    // [blargg/08-len_ctr_during_power]
    ch4.length_timer = 64 - initial_length;
}

uint8_t Apu::read_nr42() const {
    return nr42.initial_volume << Specs::Bits::Audio::NR42::INITIAL_VOLUME |
           nr42.envelope_direction << Specs::Bits::Audio::NR42::ENVELOPE_DIRECTION |
           nr42.sweep_pace << Specs::Bits::Audio::NR42::SWEEP_PACE;
}

void Apu::write_nr42(uint8_t value) {
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

uint8_t Apu::read_nr43() const {
    return nr43.clock_shift << Specs::Bits::Audio::NR43::CLOCK_SHIFT |
           nr43.lfsr_width << Specs::Bits::Audio::NR43::LFSR_WIDTH |
           nr43.clock_divider << Specs::Bits::Audio::NR43::CLOCK_DIVIDER;
}

void Apu::write_nr43(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr43.clock_shift = get_bits_range<Specs::Bits::Audio::NR43::CLOCK_SHIFT>(value);
    nr43.lfsr_width = test_bit<Specs::Bits::Audio::NR43::LFSR_WIDTH>(value);
    nr43.clock_divider = get_bits_range<Specs::Bits::Audio::NR43::CLOCK_DIVIDER>(value);
}

uint8_t Apu::read_nr44() const {
    return 0b10111111 | nr44.length_enable << Specs::Bits::Audio::NR44::LENGTH_ENABLE;
}

void Apu::write_nr44(uint8_t value) {
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
            tick_channel_length_timer(ch4, nr52.ch4);
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
        ch4.volume_sweep.pace = nr42.sweep_pace;
        ch4.volume_sweep.timer = 0;
        ch4.volume_sweep.expired = 0;

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

uint8_t Apu::read_nr50() const {
    return nr50.vin_left << Specs::Bits::Audio::NR50::VIN_LEFT |
           nr50.volume_left << Specs::Bits::Audio::NR50::VOLUME_LEFT |
           nr50.vin_right << Specs::Bits::Audio::NR50::VIN_RIGHT |
           nr50.volume_right << Specs::Bits::Audio::NR50::VOLUME_RIGHT;
}

void Apu::write_nr50(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr50.vin_left = test_bit<Specs::Bits::Audio::NR50::VIN_LEFT>(value);
    nr50.volume_left = get_bits_range<Specs::Bits::Audio::NR50::VOLUME_LEFT>(value);
    nr50.vin_right = test_bit<Specs::Bits::Audio::NR50::VIN_RIGHT>(value);
    nr50.volume_right = get_bits_range<Specs::Bits::Audio::NR50::VOLUME_RIGHT>(value);
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

uint8_t Apu::read_nr52() const {
    return 0b01110000 | nr52.enable << Specs::Bits::Audio::NR52::AUDIO_ENABLE |
           nr52.ch4 << Specs::Bits::Audio::NR52::CH4_ENABLE | nr52.ch3 << Specs::Bits::Audio::NR52::CH3_ENABLE |
           nr52.ch2 << Specs::Bits::Audio::NR52::CH2_ENABLE | nr52.ch1 << Specs::Bits::Audio::NR52::CH1_ENABLE;
}

void Apu::write_nr52(uint8_t value) {
    const bool en = test_bit<Specs::Bits::Audio::NR52::AUDIO_ENABLE>(value);
    if (en != nr52.enable) {
        en ? turn_on() : turn_off();
        nr52.enable = en;
    }
}

uint8_t Apu::read_wave_ram(uint16_t address) const {
    if (!nr52.ch3) {
        // Wave ram is accessed normally if CH3 is off.
        return wave_ram[address - Specs::Registers::Sound::WAVE0];
    }

    // When CH3 is on, reading wave ram yields the byte CH3 is currently
    // reading this T-cycle, or 0xFF if CH3 is not reading anything.
    // [blargg/09-wave_read_while_on]
    if (ticks == ch3.last_read_tick + 1) {
        return wave_ram[ch3.wave.play_position];
    }

    return 0xFF;
}

void Apu::write_wave_ram(uint16_t address, uint8_t value) {
    if (!nr52.ch3) {
        // Wave ram is accessed normally if CH3 is off.
        wave_ram[address - Specs::Registers::Sound::WAVE0] = value;
        return;
    }

    // TODO: this is a hack, check exact timing
    //  (I guess corruption should take place here, not in tick_ch3)
    ch3.pending_wave_write.pending = true;
    ch3.pending_wave_write.value = value;
}

#ifdef ENABLE_AUDIO_PCM
void Apu::update_pcm() {
    DigitalAudioSample digital_output = compute_digital_audio_sample();
    pcm12 = digital_output.ch2 << 4 | digital_output.ch1;
    pcm34 = digital_output.ch4 << 4 | digital_output.ch3;
}

uint8_t Apu::read_pcm12() const {
    return pcm12;
}

uint8_t Apu::read_pcm34() const {
    return pcm34;
}
#endif
