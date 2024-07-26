#include "docboy/audio/apu.h"

#include <cstring>

#include "docboy/timers/timers.h"

#include "docboy/bootrom/helpers.h"

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

    // Reload frequency timer
    ch1.period_timer = nr14.period_high << 8 | nr13.period_low;
    ch1.dac = true;

    ch2.period_timer = nr24.period_high << 8 | nr23.period_low;
    ch2.dac = false;
}

Apu::AudioSample Apu::compute_audio_sample() const {
    ASSERT(nr11.duty_cycle >= 0 && nr11.duty_cycle < 4);
    ASSERT(ch1.square_wave_position >= 0 && ch1.square_wave_position < 8);
    ASSERT(ch1.volume <= 0xF);

    ASSERT(nr21.duty_cycle >= 0 && nr21.duty_cycle < 4);
    ASSERT(ch2.square_wave_position >= 0 && ch2.square_wave_position < 8);
    ASSERT(ch2.volume <= 0xF);

    struct {
        int16_t ch1 {};
        int16_t ch2 {};
        int16_t ch3 {};
        int16_t ch4 {};
    } analog_output;

    // CH1
    {
        uint8_t digital_volume {};

        if (nr52.ch1) {
            ASSERT(ch1.dac);
            const bool digital_amplitude = SQUARE_WAVES[nr11.duty_cycle][ch1.square_wave_position];
            digital_volume = digital_amplitude * ch1.volume;
        }

        if (ch1.dac) {
            // Note: DAC does its work even if channel is off.
            const int16_t analog_volume = digital_to_analog_volume(digital_volume);
            analog_output.ch1 = analog_volume;
        }
    }

    // CH2
    {
        uint8_t digital_volume {};

        if (nr52.ch2) {
            ASSERT(ch2.dac);
            const bool digital_amplitude = SQUARE_WAVES[nr21.duty_cycle][ch2.square_wave_position];
            digital_volume = digital_amplitude * ch2.volume;
        }

        if (ch2.dac) {
            // Note: DAC does its work even if channel is off.
            const int16_t analog_volume = digital_to_analog_volume(digital_volume);
            analog_output.ch2 = analog_volume;
        }
    }

    // CH3
    {
        uint8_t digital_volume {};

        if (nr52.ch3 && nr32.volume) {
            ASSERT(nr30.dac);

            const uint8_t wave_index = ch3.wave_position >> 1;
            ASSERT(wave_index < wave_ram.Size);

            if (mod<2>(ch3.wave_position)) {
                // Low nibble
                digital_volume = keep_bits<4>(wave_ram[wave_index]);
            } else {
                // High nibble
                digital_volume = wave_ram[wave_index] >> 4;
            }
            ASSERT(digital_volume <= 0xF);

            ASSERT(nr32.volume > 0 && nr32.volume <= 0b11);

            // Scale by volume
            digital_volume = digital_volume >> (nr32.volume - 1);
        }

        if (nr30.dac) {
            // Note: DAC does its work even if channel is off.
            const int16_t analog_volume = digital_to_analog_volume(digital_volume);
            analog_output.ch3 = analog_volume;
        }
    }

    // CH4
    {
        uint8_t digital_volume {};

        if (nr52.ch4) {
            ASSERT(ch4.dac);

            const bool random_output = test_bit<0>(ch4.lfsr);
            digital_volume = random_output ? ch4.volume : 0;

            ASSERT(digital_volume <= 0xF);
        }

        if (ch4.dac) {
            // Note: DAC does its work even if channel is off.
            const int16_t analog_volume = digital_to_analog_volume(digital_volume);
            analog_output.ch4 = analog_volume;
        }
    }

    // Mix channels
    struct {
        struct ChanneStereoAnalogOutput {
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

void Apu::tick_t0() {
    // Do nothing if APU is off (but advance the sampler anyhow).
    if (!nr52.enable) {
        tick_sampler();
        return;
    }

    ASSERT(!(nr52.ch1 && !ch1.dac));
    ASSERT(!(nr52.ch2 && !ch2.dac));
    ASSERT(!(nr52.ch3 && !nr30.dac));

    // Update DIV APU
    const bool div_bit_4 = test_bit<4>(timers.read_div());
    if (prev_div_bit_4 && !div_bit_4) {
        // Increase DIV-APU each time DIV[4] has a falling edge (~512Hz)
        div_apu++;

        if (mod<2>(div_apu) == 0) {
            // Increase length timer
            if (nr52.ch1 && nr14.length_enable) {
                if (++ch1.length_timer >= 64) {
                    // Length timer expired: turn off the channel
                    nr52.ch1 = false;
                }
            }

            // Increase length timer
            if (nr52.ch2 && nr24.length_enable) {
                if (++ch2.length_timer >= 64) {
                    // Length timer expired: turn off the channel
                    nr52.ch2 = false;
                }
            }

            // Increase length timer
            if (nr52.ch3 && nr34.length_enable) {
                if (++ch3.length_timer >= 64) {
                    // Length timer expired: turn off the channel
                    nr52.ch3 = false;
                }
            }

            // Increase length timer
            if (nr52.ch4 && nr44.length_enable) {
                if (++ch4.length_timer >= 64) {
                    // Length timer expired: turn off the channel
                    nr52.ch4 = false;
                }
            }

            if (mod<4>(div_apu) == 0) {
                if (nr52.ch1 && nr10.pace /* 0 disables envelope */) {
                    if (++ch1.sweep_counter >= nr10.pace) {
                        // Update period:
                        // P_t+1 = P_t + P_t / (2^pace)

                        uint16_t period = nr14.period_high << 8 | nr13.period_low;

                        int32_t new_period = period + (nr10.direction ? 1 : -1) * (period >> nr10.step);
                        if (new_period >= 2048) {
                            // Period overflow: turn off the channel
                            nr52.ch1 = false;
                        }
                        if (new_period < 0) {
                            new_period = 0;
                        }

                        nr14.period_high = get_bits_range<11, 8>(new_period);
                        nr13.period_low = keep_bits<8>(new_period);

                        // Reset counter?
                        ch1.sweep_counter = 0;
                    }
                }

                if (mod<8>(div_apu) == 0) {
                    if (nr52.ch1 && ch1.sweep_pace /* 0 disables envelope */) {
                        // Update envelope
                        if (++ch1.envelope_counter >= ch1.sweep_pace) {
                            // Increase/decrease volume (if possible)
                            if (ch1.envelope_direction) {
                                if (ch1.volume < 0xF) {
                                    ++ch1.volume;
                                }
                            } else {
                                if (ch1.volume > 0) {
                                    --ch1.volume;
                                }
                            }

                            // Reset counter?
                            ch1.envelope_counter = 0;
                        }
                    }

                    if (nr52.ch2 && ch2.sweep_pace /* 0 disables envelope */) {
                        // Update envelope
                        if (++ch2.envelope_counter >= ch2.sweep_pace) {
                            // Increase/decrease volume (if possible)
                            if (ch2.envelope_direction) {
                                if (ch2.volume < 0xF) {
                                    ++ch2.volume;
                                }
                            } else {
                                if (ch2.volume > 0) {
                                    --ch2.volume;
                                }
                            }

                            // Reset counter?
                            ch2.envelope_counter = 0;
                        }
                    }

                    if (nr52.ch4 && ch4.sweep_pace /* 0 disables envelope */) {
                        // Update envelope
                        if (++ch4.envelope_counter >= ch4.sweep_pace) {
                            // Increase/decrease volume (if possible)
                            if (ch4.envelope_direction) {
                                if (ch4.volume < 0xF) {
                                    ++ch4.volume;
                                }
                            } else {
                                if (ch4.volume > 0) {
                                    --ch4.volume;
                                }
                            }

                            // Reset counter?
                            ch4.envelope_counter = 0;
                        }
                    }
                }
            }
        }
    }
    prev_div_bit_4 = div_bit_4;

    // Advance period timer
    if (nr52.ch1) {
        if (++ch1.period_timer == 2048) {
            // Advance square wave position
            ch1.square_wave_position = mod<8>(ch1.square_wave_position + 1);

            // Reload period timer
            ch1.period_timer = nr14.period_high << 8 | nr13.period_low;
            ASSERT(ch1.period_timer < 2048);
        }
    }

    if (nr52.ch2) {
        if (++ch2.period_timer == 2048) {
            // Advance square wave position
            ch2.square_wave_position = mod<8>(ch2.square_wave_position + 1);

            // Reload period timer
            ch2.period_timer = nr24.period_high << 8 | nr23.period_low;
            ASSERT(ch2.period_timer < 2048);
        }
    }

    if (nr52.ch3) {
        ch3.period_timer += 2;
        if (ch3.period_timer >= 2048) {
            // Advance square wave position
            ch3.wave_position = mod<32>(ch3.wave_position + 1);

            // Reload period timer
            ch3.period_timer = nr34.period_high << 8 | nr33.period_low;
            //            ASSERT(mod<2>(ch3.period_timer) == 0);
            ASSERT(ch3.period_timer < 2048);
        }
    }

    if (nr52.ch4) {
        uint8_t D = nr43.clock_divider ? 2 * nr43.clock_divider : 1;

        if (++ch4.period_timer >= 2 * D << nr43.clock_shift) {
            const bool b = test_bit<0>(ch4.lfsr) == test_bit<1>(ch4.lfsr);
            set_bit<15>(ch4.lfsr, b);
            if (nr43.lfsr_width) {
                set_bit<7>(ch4.lfsr, b);
            }
            ch4.lfsr = ch4.lfsr >> 1;
            ch4.period_timer = 0;
        }
    }

    tick_sampler();
}

void Apu::tick_t1() {
    tick_sampler();
}

void Apu::tick_t2() {
    tick_sampler();
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
}

void Apu::save_state(Parcel& parcel) const {
    parcel.write_uint64(ticks);

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
}

void Apu::load_state(Parcel& parcel) {
    ticks = parcel.read_uint64();
    next_tick_sample = static_cast<double>(ticks) + sample_period;

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
    nr14.trigger = parcel.read_uint8();
    nr21.duty_cycle = parcel.read_uint8();
    nr21.initial_length_timer = parcel.read_uint8();
    nr22.initial_volume = parcel.read_uint8();
    nr22.envelope_direction = parcel.read_bool();
    nr22.sweep_pace = parcel.read_uint8();
    nr23.period_low = parcel.read_uint8();
    nr24.trigger = parcel.read_bool();
    nr24.length_enable = parcel.read_bool();
    nr24.trigger = parcel.read_uint8();
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
}

uint8_t Apu::read_nr11() const {
    return 0b00111111 | nr11.duty_cycle << Specs::Bits::Audio::NR11::DUTY_CYCLE;
}

void Apu::write_nr11(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr11.duty_cycle = get_bits_range<Specs::Bits::Audio::NR11::DUTY_CYCLE>(value);
    nr11.initial_length_timer = get_bits_range<Specs::Bits::Audio::NR11::INITIAL_LENGTH_TIMER>(value);
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

    nr14.trigger = test_bit<Specs::Bits::Audio::NR14::TRIGGER>(value);
    nr14.length_enable = test_bit<Specs::Bits::Audio::NR14::LENGTH_ENABLE>(value);
    nr14.period_high = get_bits_range<Specs::Bits::Audio::NR14::PERIOD>(value);
    ASSERT(nr14.period_high < 8);

    if (nr14.trigger && ch1.dac) {
        // Any write with Trigger bit set and DAC enabled turns on the channel
        nr52.ch1 = true;

        // TODO: where are these reset here?
        ch1.length_timer = nr11.initial_length_timer;
        ch1.period_timer = nr14.period_high << 8 | nr13.period_low;
        ch1.envelope_counter = 0;
        ch1.volume = nr12.initial_volume;
        ch1.envelope_direction = nr12.envelope_direction;
        ch1.sweep_pace = nr12.sweep_pace;
    }
}

uint8_t Apu::read_nr21() const {
    return 0b00111111 | nr21.duty_cycle << Specs::Bits::Audio::NR21::DUTY_CYCLE;
}

void Apu::write_nr21(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr21.duty_cycle = get_bits_range<Specs::Bits::Audio::NR21::DUTY_CYCLE>(value);
    nr21.initial_length_timer = get_bits_range<Specs::Bits::Audio::NR21::INITIAL_LENGTH_TIMER>(value);
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

    nr24.trigger = test_bit<Specs::Bits::Audio::NR24::TRIGGER>(value);
    nr24.length_enable = test_bit<Specs::Bits::Audio::NR24::LENGTH_ENABLE>(value);
    nr24.period_high = get_bits_range<Specs::Bits::Audio::NR24::PERIOD>(value);
    ASSERT(nr24.period_high < 8);

    if (nr24.trigger && ch2.dac) {
        // Any write with Trigger bit set and DAC enabled turns on the channel
        nr52.ch2 = true;

        // TODO: where are these reset here?
        ch2.length_timer = nr21.initial_length_timer;
        ch2.period_timer = nr24.period_high << 8 | nr23.period_low;
        ch2.envelope_counter = 0;
        ch2.volume = nr22.initial_volume;
        ch2.envelope_direction = nr22.envelope_direction;
        ch2.sweep_pace = nr22.sweep_pace;
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
    if (!nr52.enable) {
        return;
    }

    nr31.initial_length_timer = value;
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

    nr34.trigger = test_bit<Specs::Bits::Audio::NR34::TRIGGER>(value);
    nr34.length_enable = test_bit<Specs::Bits::Audio::NR34::LENGTH_ENABLE>(value);
    nr34.period_high = get_bits_range<Specs::Bits::Audio::NR34::PERIOD>(value);

    if (nr34.trigger && nr30.dac) {
        // Any write with Trigger bit set and DAC enabled turns on the channel
        nr52.ch3 = true;

        ch3.length_timer = nr31.initial_length_timer;
        ch3.wave_position = 1;
    }
}

uint8_t Apu::read_nr41() const {
    return 0xFF;
}

void Apu::write_nr41(uint8_t value) {
    if (!nr52.enable) {
        return;
    }

    nr41.initial_length_timer = get_bits_range<Specs::Bits::Audio::NR41::INITIAL_LENGTH_TIMER>(value);
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

    nr44.trigger = test_bit<Specs::Bits::Audio::NR44::TRIGGER>(value);
    nr44.length_enable = test_bit<Specs::Bits::Audio::NR44::LENGTH_ENABLE>(value);

    if (nr44.trigger && ch4.dac) {
        // Any write with Trigger bit set and DAC enabled turns on the channel
        nr52.ch4 = true;

        // TODO: where are these reset here?
        ch4.length_timer = nr41.initial_length_timer;
        ch4.envelope_counter = 0;
        ch4.volume = nr42.initial_volume;
        ch4.envelope_direction = nr42.envelope_direction;
        ch4.sweep_pace = nr42.sweep_pace;
        ch4.lfsr = 0;
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