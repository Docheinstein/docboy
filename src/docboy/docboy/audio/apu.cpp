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
}

Apu::Apu(Timers& timers) :
    timers {timers} {
    reset();
}

void Apu::set_audio_callback(std::function<void(const int16_t*, uint32_t)>&& callback) {
    audio_callback = std::move(callback);
}

void Apu::reset() {
    // Reset I/O registers
    nr10 = 0b10000000;
    nr11 = if_bootrom_else(0, 0b10111111);
    nr12 = if_bootrom_else(0, 0b11110011);
    nr13 = if_bootrom_else(0, 0b11111111);
    nr14 = 0b10111111; // TODO: B8 or BF?
    nr21.duty_cycle = false;
    nr21.initial_length_timer = if_bootrom_else(0, 0b00111111);
    nr22.initial_volume = false;
    nr22.envelope_direction = false;
    nr22.sweep_pace = false;
    nr23 = if_bootrom_else(0, 0b11111111);
    nr24.trigger = true;
    nr24.length_enable = false;
    nr24.period = 0b00111111;
    nr30 = 0b01111111;
    nr31 = if_bootrom_else(0, 0b11111111);
    nr32 = 0b10011111;
    nr33 = if_bootrom_else(0, 0b11111111);
    nr34 = 0b10111111; // TODO: B8 or BF?
    nr41 = if_bootrom_else(0, 0b11111111);
    nr42 = 0;
    nr43 = 0;
    nr44 = 0b10111111; // TODO: B8 or BF?
    nr50 = if_bootrom_else(0, 0b01110111);
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
    ch2.period_timer = nr24.period << 8 | nr23;
    ch2.dac = false;
    memset(samples, 0, sizeof(samples));
}

int16_t Apu::compute_audio_sample() const {
    if (!(nr52.ch2 && ch2.dac)) {
        return 0;
    }

    ASSERT(square_wave_position >= 0 && square_wave_position < 8);
    ASSERT(nr21.duty_cycle >= 0 && nr21.duty_cycle < 4);

    const bool digital_amplitude = SQUARE_WAVES[nr21.duty_cycle][square_wave_position];
    const int16_t volume = 16000; // TODO

    const int16_t result = digital_amplitude * volume;

    return result;
}

void Apu::tick_t0() {
    // Do nothing if APU is off
    if (!nr52.enable) {
        return;
    }

    // Update DIV APU
    const bool div_bit_4 = test_bit<4>(timers.div);
    if (prev_div_bit_4 && !div_bit_4) {
        // Increase DIV-APU each time DIV[4] has a falling edge
        div_apu++;

        if (mod<2>(div_apu) == 0) {
            // Increase length timer
            if (++ch2.length_timer == nr21.initial_length_timer) {
                if (nr24.length_enable) {
                    // Length timer expired: turn off the channel
                    nr52.ch2 = false;
                }
            }
        }
    }
    prev_div_bit_4 = div_bit_4;

    // Advance period timer
    if (++ch2.period_timer == 2048) {
        // Advance square wave position
        square_wave_position = mod<8>(square_wave_position + 1);

        // Reload period timer
        ch2.period_timer = nr24.period << 8 | nr23;
        ASSERT(ch2.period_timer < 2048);
    }

    // Handle sampling
    ticks_since_last_sample += 4;
    static_assert(mod<4>(TICKS_BETWEEN_SAMPLES) == 0);

    if (ticks_since_last_sample == TICKS_BETWEEN_SAMPLES) {
        ticks_since_last_sample = 0;

        ASSERT(sample_index < SAMPLES_PER_FRAME);
        samples[sample_index] = compute_audio_sample();

        sample_index = sample_index + 1;
        if (sample_index == SAMPLES_PER_FRAME) {
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
    parcel.write_uint8(nr10);
    parcel.write_uint8(nr11);
    parcel.write_uint8(nr12);
    parcel.write_uint8(nr13);
    parcel.write_uint8(nr14);
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
    parcel.write_uint8(nr50);
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
    nr10 = parcel.read_uint8();
    nr11 = parcel.read_uint8();
    nr12 = parcel.read_uint8();
    nr13 = parcel.read_uint8();
    nr14 = parcel.read_uint8();
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
    nr50 = parcel.read_uint8();
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

void Apu::write_nr10(uint8_t value) {
    nr10 = 0b10000000 | value;
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
           nr22.initial_volume << Specs::Bits::Audio::NR22::SWEEP_PACE.lsb;
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

    if (nr24.trigger && ch2.dac) {
        // Any write with Trigger bit set and DAC enabled turns on the channel
        nr52.ch2 = true;
    }
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