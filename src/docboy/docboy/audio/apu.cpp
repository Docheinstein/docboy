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
    return static_cast<int16_t>(analog_volume) / 2 /* n channels ? */;
}

inline int16_t scale_analog_volume_by_master_volume(int16_t input_analog_volume, uint8_t master_volume) {
    ASSERT(master_volume < 8);
    int32_t output_analog_volume = input_analog_volume * (1 + master_volume) / 8;
    ASSERT(output_analog_volume >= INT16_MIN && output_analog_volume <= INT16_MAX);
    return static_cast<int16_t>(output_analog_volume);
}
} // namespace

Apu::Apu(Timers& timers) :
    timers {timers} {
    reset();
}

void Apu::set_audio_callback(std::function<void(const int16_t*, uint32_t)>&& callback) {
    audio_callback = std::move(callback);
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
    nr13 = if_bootrom_else(0, 0b11111111);
    nr14.trigger = true;
    nr14.length_enable = false;
    nr14.period = 0b111;
    nr21.duty_cycle = 0;
    nr21.initial_length_timer = if_bootrom_else(0, 0b111111);
    nr22.initial_volume = false;
    nr22.envelope_direction = false;
    nr22.sweep_pace = false;
    nr23 = if_bootrom_else(0, 0b11111111);
    nr24.trigger = true;
    nr24.length_enable = false;
    nr24.period = 0b111;
    nr30 = 0b01111111;
    nr31 = if_bootrom_else(0, 0b11111111);
    nr32 = 0b10011111;
    nr33 = if_bootrom_else(0, 0b11111111);
    nr34 = 0b10111111; // TODO: B8 or BF?
    nr41 = if_bootrom_else(0, 0b11111111);
    nr42 = 0;
    nr43 = 0;
    nr44 = 0b10111111; // TODO: B8 or BF?
    nr50.vin_left = false;
    nr50.volume_left = if_bootrom_else(0, 0b111);
    nr50.vin_right = false;
    nr50.volume_right = if_bootrom_else(0, 0b111);
    nr51 = if_bootrom_else(0, 0b11110011);
    nr52.enable = true;
    nr52.ch4 = false;
    nr52.ch3 = false;
    nr52.ch2 = false;
    nr52.ch1 = true;
    wave0 = 0;
    wave1 = 0;
    wave2 = 0;
    wave3 = 0;
    wave4 = 0;
    wave5 = 0;
    wave6 = 0;
    wave7 = 0;
    wave8 = 0;
    wave9 = 0;
    waveA = 0;
    waveB = 0;
    waveC = 0;
    waveD = 0;
    waveE = 0;
    waveF = 0;

    // Reload frequency timer
    ch1.period_timer = nr14.period << 8 | nr13;
    ch1.dac = true;

    ch2.period_timer = nr24.period << 8 | nr23;
    ch2.dac = false;

    memset(samples, 0, sizeof(samples));
}

Apu::AudioSample Apu::compute_audio_sample() const {
    ASSERT(nr11.duty_cycle >= 0 && nr11.duty_cycle < 4);
    ASSERT(ch1.square_wave_position >= 0 && ch1.square_wave_position < 8);
    ASSERT(ch1.volume <= 0xF);

    ASSERT(nr21.duty_cycle >= 0 && nr21.duty_cycle < 4);
    ASSERT(ch2.square_wave_position >= 0 && ch2.square_wave_position < 8);
    ASSERT(ch2.volume <= 0xF);

    int32_t output {};

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
            output += analog_volume;
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
            output += analog_volume;
        }
    }

    ASSERT(volume >= 0.0f && volume <= 1.0f);
    output = static_cast<int32_t>(output * volume);

    ASSERT(output >= INT16_MIN && output <= INT16_MAX);
    return {
        scale_analog_volume_by_master_volume(output, nr50.volume_left),
        scale_analog_volume_by_master_volume(output, nr50.volume_right),
    };
}

void Apu::tick_t0() {
    // Do nothing if APU is off
    if (!nr52.enable) {
        return;
    }

    ASSERT(!(nr52.ch1 && !ch1.dac));
    ASSERT(!(nr52.ch2 && !ch2.dac));

    const bool ch1_on = nr52.ch1;
    const bool ch2_on = nr52.ch2;

    // Update DIV APU
    const bool div_bit_4 = test_bit<4>(timers.read_div());
    if (prev_div_bit_4 && !div_bit_4) {
        // Increase DIV-APU each time DIV[4] has a falling edge (~512Hz)
        div_apu++;

        if (mod<2>(div_apu) == 0) {
            // Increase length timer
            if (ch1_on && nr14.length_enable) {
                if (++ch1.length_timer == nr11.initial_length_timer) {
                    // Length timer expired: turn off the channel
                    nr52.ch1 = false;
                }
            }

            // Increase length timer
            if (ch2_on && nr24.length_enable) {
                if (++ch2.length_timer == nr21.initial_length_timer) {
                    // Length timer expired: turn off the channel
                    nr52.ch2 = false;
                }
            }

            if (mod<4>(div_apu) == 0) {
                if (ch1_on && nr10.pace /* 0 disables envelope */) {
                    if (++ch1.sweep_counter >= nr10.pace) {
                        // Update period:
                        // P_t+1 = P_t + P_t / (2^pace)

                        uint16_t period = nr14.period << 8 | nr13;

                        int32_t new_period = period + (nr10.direction ? 1 : -1) * (period >> nr10.step);
                        if (new_period >= 2048) {
                            // Period overflow: turn off the channel
                            nr52.ch1 = false;
                        }
                        if (new_period < 0) {
                            new_period = 0;
                        }

                        nr14.period = get_bits_range<11, 8>(new_period);
                        nr13 = keep_bits<8>(new_period);

                        // Reset counter?
                        ch1.sweep_counter = 0;
                    }
                }

                if (mod<8>(div_apu) == 0) {
                    if (ch1_on && ch1.sweep_pace /* 0 disables envelope */) {
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

                    if (ch2_on && ch2.sweep_pace /* 0 disables envelope */) {
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
                }
            }
        }
    }
    prev_div_bit_4 = div_bit_4;

    // Advance period timer
    if (ch1_on) {
        if (++ch1.period_timer == 2048) {
            // Advance square wave position
            ch1.square_wave_position = mod<8>(ch1.square_wave_position + 1);

            // Reload period timer
            ch1.period_timer = nr14.period << 8 | nr13;
            ASSERT(ch1.period_timer < 2048);
        }
    }

    if (ch2_on) {
        if (++ch2.period_timer == 2048) {
            // Advance square wave position
            ch2.square_wave_position = mod<8>(ch2.square_wave_position + 1);

            // Reload period timer
            ch2.period_timer = nr24.period << 8 | nr23;
            ASSERT(ch2.period_timer < 2048);
        }
    }

    // Handle sampling
    ticks_since_last_sample += 4;
    static_assert(mod<4>(TICKS_BETWEEN_SAMPLES) == 0);

    if (ticks_since_last_sample == TICKS_BETWEEN_SAMPLES) {
        ticks_since_last_sample = 0;

        ASSERT(sample_index < SAMPLES_PER_FRAME * NUM_CHANNELS);
        AudioSample sample = compute_audio_sample();
        samples[sample_index++] = sample.left;
        samples[sample_index++] = sample.right;

        if (sample_index == SAMPLES_PER_FRAME * NUM_CHANNELS) {
            sample_index = 0;
            if (audio_callback) {
                audio_callback(samples, SAMPLES_PER_FRAME);
            }
        }
    }
}

void Apu::tick_t1() {
}

void Apu::tick_t2() {
}

void Apu::tick_t3() {
}

void Apu::turn_on() {
    reset();
}

void Apu::turn_off() {
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
    parcel.write_uint8(nr13);
    parcel.write_bool(nr14.trigger);
    parcel.write_bool(nr14.length_enable);
    parcel.write_uint8(nr14.period);
    parcel.write_uint8(nr21.duty_cycle);
    parcel.write_uint8(nr21.initial_length_timer);
    parcel.write_uint8(nr22.initial_volume);
    parcel.write_bool(nr22.envelope_direction);
    parcel.write_uint8(nr22.sweep_pace);
    parcel.write_uint8(nr23);
    parcel.write_bool(nr24.trigger);
    parcel.write_bool(nr24.length_enable);
    parcel.write_uint8(nr24.period);
    parcel.write_uint8(nr30);
    parcel.write_uint8(nr31);
    parcel.write_uint8(nr32);
    parcel.write_uint8(nr33);
    parcel.write_uint8(nr34);
    parcel.write_uint8(nr41);
    parcel.write_uint8(nr42);
    parcel.write_uint8(nr43);
    parcel.write_uint8(nr44);
    parcel.write_bool(nr50.vin_left);
    parcel.write_uint8(nr50.volume_left);
    parcel.write_bool(nr50.vin_right);
    parcel.write_uint8(nr50.volume_right);
    parcel.write_uint8(nr51);
    parcel.write_bool(nr52.enable);
    parcel.write_bool(nr52.ch4);
    parcel.write_bool(nr52.ch3);
    parcel.write_bool(nr52.ch2);
    parcel.write_bool(nr52.ch1);
    parcel.write_uint8(wave0);
    parcel.write_uint8(wave1);
    parcel.write_uint8(wave2);
    parcel.write_uint8(wave3);
    parcel.write_uint8(wave4);
    parcel.write_uint8(wave5);
    parcel.write_uint8(wave6);
    parcel.write_uint8(wave7);
    parcel.write_uint8(wave8);
    parcel.write_uint8(wave9);
    parcel.write_uint8(waveA);
    parcel.write_uint8(waveB);
    parcel.write_uint8(waveC);
    parcel.write_uint8(waveD);
    parcel.write_uint8(waveE);
    parcel.write_uint8(waveF);
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
    nr13 = parcel.read_uint8();
    nr14.trigger = parcel.read_bool();
    nr14.length_enable = parcel.read_bool();
    nr14.trigger = parcel.read_uint8();
    nr21.duty_cycle = parcel.read_uint8();
    nr21.initial_length_timer = parcel.read_uint8();
    nr22.initial_volume = parcel.read_uint8();
    nr22.envelope_direction = parcel.read_bool();
    nr22.sweep_pace = parcel.read_uint8();
    nr23 = parcel.read_uint8();
    nr24.trigger = parcel.read_bool();
    nr24.length_enable = parcel.read_bool();
    nr24.trigger = parcel.read_uint8();
    nr30 = parcel.read_uint8();
    nr31 = parcel.read_uint8();
    nr32 = parcel.read_uint8();
    nr33 = parcel.read_uint8();
    nr34 = parcel.read_uint8();
    nr41 = parcel.read_uint8();
    nr42 = parcel.read_uint8();
    nr43 = parcel.read_uint8();
    nr44 = parcel.read_uint8();
    nr50.vin_left = parcel.read_bool();
    nr50.volume_left = parcel.read_uint8();
    nr50.vin_right = parcel.read_bool();
    nr50.volume_right = parcel.read_uint8();
    nr51 = parcel.read_uint8();
    nr52.enable = parcel.read_bool();
    nr52.ch4 = parcel.read_bool();
    nr52.ch3 = parcel.read_bool();
    nr52.ch2 = parcel.read_bool();
    nr52.ch1 = parcel.read_bool();
    wave0 = parcel.read_uint8();
    wave1 = parcel.read_uint8();
    wave2 = parcel.read_uint8();
    wave3 = parcel.read_uint8();
    wave4 = parcel.read_uint8();
    wave5 = parcel.read_uint8();
    wave6 = parcel.read_uint8();
    wave7 = parcel.read_uint8();
    wave8 = parcel.read_uint8();
    wave9 = parcel.read_uint8();
    waveA = parcel.read_uint8();
    waveB = parcel.read_uint8();
    waveC = parcel.read_uint8();
    waveD = parcel.read_uint8();
    waveE = parcel.read_uint8();
    waveF = parcel.read_uint8();
}

void Apu::write_nr30(uint8_t value) {
    nr30 = 0b01111111 | value;
}

void Apu::write_nr32(uint8_t value) {
    nr30 = 0b10011111 | value;
}

void Apu::write_nr41(uint8_t value) {
    nr41 = 0b11000000 | value;
}

void Apu::write_nr44(uint8_t value) {
    nr44 = 0b00111111 | value;
}

uint8_t Apu::read_nr10() const {
    return 0b10000000 | nr10.pace << Specs::Bits::Audio::NR10::PACE.lsb |
           nr10.direction << Specs::Bits::Audio::NR10::DIRECTION | nr10.step << Specs::Bits::Audio::NR10::STEP.lsb;
}

void Apu::write_nr10(uint8_t value) {
    nr10.pace = get_bits_range<Specs::Bits::Audio::NR10::PACE>(value);
    nr10.direction = test_bit<Specs::Bits::Audio::NR10::DIRECTION>(value);
    nr10.step = get_bits_range<Specs::Bits::Audio::NR10::STEP>(value);
}

uint8_t Apu::read_nr11() const {
    return nr11.duty_cycle << Specs::Bits::Audio::NR11::DUTY_CYCLE.lsb |
           nr11.initial_length_timer << Specs::Bits::Audio::NR11::INITIAL_LENGTH_TIMER.lsb;
}

void Apu::write_nr11(uint8_t value) {
    nr11.duty_cycle = get_bits_range<Specs::Bits::Audio::NR11::DUTY_CYCLE>(value);
    nr11.initial_length_timer = get_bits_range<Specs::Bits::Audio::NR11::INITIAL_LENGTH_TIMER>(value);
}

uint8_t Apu::read_nr12() const {
    return nr12.initial_volume << Specs::Bits::Audio::NR12::INITIAL_VOLUME.lsb |
           nr12.envelope_direction << Specs::Bits::Audio::NR12::ENVELOPE_DIRECTION |
           nr12.sweep_pace << Specs::Bits::Audio::NR12::SWEEP_PACE.lsb;
}

void Apu::write_nr12(uint8_t value) {
    nr12.initial_volume = get_bits_range<Specs::Bits::Audio::NR12::INITIAL_VOLUME>(value);
    nr12.envelope_direction = test_bit<Specs::Bits::Audio::NR12::ENVELOPE_DIRECTION>(value);
    nr12.sweep_pace = get_bits_range<Specs::Bits::Audio::NR12::SWEEP_PACE>(value);

    ch1.dac = nr12.initial_volume | nr12.envelope_direction;
    if (!ch1.dac) {
        // If the DAC is turned off the channel is disabled as well
        nr52.ch1 = false;
    }
}

uint8_t Apu::read_nr14() const {
    return 0b00111000 | nr14.trigger << Specs::Bits::Audio::NR14::TRIGGER |
           nr14.length_enable << Specs::Bits::Audio::NR14::LENGTH_ENABLE |
           nr14.period << Specs::Bits::Audio::NR14::PERIOD.lsb;
}

void Apu::write_nr14(uint8_t value) {
    nr14.trigger = test_bit<Specs::Bits::Audio::NR14::TRIGGER>(value);
    nr14.length_enable = test_bit<Specs::Bits::Audio::NR14::LENGTH_ENABLE>(value);
    nr14.period = get_bits_range<Specs::Bits::Audio::NR14::PERIOD>(value);
    ASSERT(nr14.period < 8);

    if (nr14.trigger && ch1.dac) {
        // Any write with Trigger bit set and DAC enabled turns on the channel
        nr52.ch1 = true;

        // TODO: where are these reset here?
        ch1.length_timer = 0;
        ch1.period_timer = nr14.period << 8 | nr13;
        ch1.envelope_counter = 0;
        ch1.volume = nr12.initial_volume;
        ch1.envelope_direction = nr12.envelope_direction;
        ch1.sweep_pace = nr12.sweep_pace;
    }
}

uint8_t Apu::read_nr21() const {
    return nr21.duty_cycle << Specs::Bits::Audio::NR21::DUTY_CYCLE.lsb |
           nr21.initial_length_timer << Specs::Bits::Audio::NR21::INITIAL_LENGTH_TIMER.lsb;
}

void Apu::write_nr21(uint8_t value) {
    nr21.duty_cycle = get_bits_range<Specs::Bits::Audio::NR21::DUTY_CYCLE>(value);
    nr21.initial_length_timer = get_bits_range<Specs::Bits::Audio::NR21::INITIAL_LENGTH_TIMER>(value);
}

uint8_t Apu::read_nr22() const {
    return nr22.initial_volume << Specs::Bits::Audio::NR22::INITIAL_VOLUME.lsb |
           nr22.envelope_direction << Specs::Bits::Audio::NR22::ENVELOPE_DIRECTION |
           nr22.sweep_pace << Specs::Bits::Audio::NR22::SWEEP_PACE.lsb;
}

void Apu::write_nr22(uint8_t value) {
    nr22.initial_volume = get_bits_range<Specs::Bits::Audio::NR22::INITIAL_VOLUME>(value);
    nr22.envelope_direction = test_bit<Specs::Bits::Audio::NR22::ENVELOPE_DIRECTION>(value);
    nr22.sweep_pace = get_bits_range<Specs::Bits::Audio::NR22::SWEEP_PACE>(value);

    ch2.dac = nr22.initial_volume | nr22.envelope_direction;
    if (!ch2.dac) {
        // If the DAC is turned off the channel is disabled as well
        nr52.ch2 = false;
    }
}

uint8_t Apu::read_nr24() const {
    return 0b00111000 | nr24.trigger << Specs::Bits::Audio::NR24::TRIGGER |
           nr24.length_enable << Specs::Bits::Audio::NR24::LENGTH_ENABLE |
           nr24.period << Specs::Bits::Audio::NR24::PERIOD.lsb;
}

void Apu::write_nr24(uint8_t value) {
    nr24.trigger = test_bit<Specs::Bits::Audio::NR24::TRIGGER>(value);
    nr24.length_enable = test_bit<Specs::Bits::Audio::NR24::LENGTH_ENABLE>(value);
    nr24.period = get_bits_range<Specs::Bits::Audio::NR24::PERIOD>(value);
    ASSERT(nr24.period < 8);

    if (nr24.trigger && ch2.dac) {
        // Any write with Trigger bit set and DAC enabled turns on the channel
        nr52.ch2 = true;

        // TODO: where are these reset here?
        ch2.length_timer = 0;
        ch2.period_timer = nr24.period << 8 | nr23;
        ch2.envelope_counter = 0;
        ch2.volume = nr22.initial_volume;
        ch2.envelope_direction = nr22.envelope_direction;
        ch2.sweep_pace = nr22.sweep_pace;
    }
}

uint8_t Apu::read_nr50() const {
    return nr50.vin_left << Specs::Bits::Audio::NR50::VIN_LEFT |
           nr50.volume_left << Specs::Bits::Audio::NR50::VOLUME_LEFT.lsb |
           nr50.vin_right << Specs::Bits::Audio::NR50::VIN_RIGHT |
           nr50.volume_right << Specs::Bits::Audio::NR50::VOLUME_RIGHT.lsb;
}

void Apu::write_nr50(uint8_t value) {
    nr50.vin_left = test_bit<Specs::Bits::Audio::NR50::VIN_LEFT>(value);
    nr50.volume_left = get_bits_range<Specs::Bits::Audio::NR50::VOLUME_LEFT>(value);
    nr50.vin_right = test_bit<Specs::Bits::Audio::NR50::VIN_RIGHT>(value);
    nr50.volume_right = get_bits_range<Specs::Bits::Audio::NR50::VOLUME_RIGHT>(value);
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

void Apu::set_volume(float v) {
    ASSERT(volume >= 0.0f && volume <= 1.0f);
    volume = v;
}
