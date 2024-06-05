#ifndef SOUND_H
#define SOUND_H

#include "docboy/memory/byte.h"

class SoundIO {
public:
    void write_nr10(uint8_t value) {
        nr10 = 0b10000000 | value;
    }

    void write_nr30(uint8_t value) {
        nr30 = 0b01111111 | value;
    }

    void write_nr32(uint8_t value) {
        nr30 = 0b10011111 | value;
    }

    void write_nr41(uint8_t value) {
        nr41 = 0b11000000 | value;
    }

    void write_nr44(uint8_t value) {
        nr44 = 0b00111111 | value;
    }

    void write_nr52(uint8_t value) {
        nr52 = 0b01110000 | value;
    }

    void save_state(Parcel& parcel) const {
        parcel.write_uint8(nr10);
        parcel.write_uint8(nr11);
        parcel.write_uint8(nr12);
        parcel.write_uint8(nr13);
        parcel.write_uint8(nr14);
        parcel.write_uint8(nr21);
        parcel.write_uint8(nr22);
        parcel.write_uint8(nr23);
        parcel.write_uint8(nr24);
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
        parcel.write_uint8(nr52);
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

    void load_state(Parcel& parcel) {
        nr10 = parcel.read_uint8();
        nr11 = parcel.read_uint8();
        nr12 = parcel.read_uint8();
        nr13 = parcel.read_uint8();
        nr14 = parcel.read_uint8();
        nr21 = parcel.read_uint8();
        nr22 = parcel.read_uint8();
        nr23 = parcel.read_uint8();
        nr24 = parcel.read_uint8();
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
        nr52 = parcel.read_uint8();
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

    void reset() {
        nr10 = 0b10000000;
        nr11 = if_bootrom_else(0, 0b10111111);
        nr12 = if_bootrom_else(0, 0b11110011);
        nr13 = if_bootrom_else(0, 0b11111111);
        nr14 = 0b10111111; // TODO: B8 or BF?
        nr21 = if_bootrom_else(0, 0b00111111);
        nr22 = 0;
        nr23 = if_bootrom_else(0, 0b11111111);
        nr24 = 0b10111111; // TODO: B8 or BF?
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
        nr52 = if_bootrom_else(0, 0b11110001);
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
    };

    byte nr10 {make_byte(Specs::Registers::Sound::NR10)};
    byte nr11 {make_byte(Specs::Registers::Sound::NR11)};
    byte nr12 {make_byte(Specs::Registers::Sound::NR12)};
    byte nr13 {make_byte(Specs::Registers::Sound::NR13)};
    byte nr14 {make_byte(Specs::Registers::Sound::NR14)};
    byte nr21 {make_byte(Specs::Registers::Sound::NR21)};
    byte nr22 {make_byte(Specs::Registers::Sound::NR22)};
    byte nr23 {make_byte(Specs::Registers::Sound::NR23)};
    byte nr24 {make_byte(Specs::Registers::Sound::NR24)};
    byte nr30 {make_byte(Specs::Registers::Sound::NR30)};
    byte nr31 {make_byte(Specs::Registers::Sound::NR31)};
    byte nr32 {make_byte(Specs::Registers::Sound::NR32)};
    byte nr33 {make_byte(Specs::Registers::Sound::NR33)};
    byte nr34 {make_byte(Specs::Registers::Sound::NR34)};
    byte nr41 {make_byte(Specs::Registers::Sound::NR41)};
    byte nr42 {make_byte(Specs::Registers::Sound::NR42)};
    byte nr43 {make_byte(Specs::Registers::Sound::NR43)};
    byte nr44 {make_byte(Specs::Registers::Sound::NR44)};
    byte nr50 {make_byte(Specs::Registers::Sound::NR50)};
    byte nr51 {make_byte(Specs::Registers::Sound::NR51)};
    byte nr52 {make_byte(Specs::Registers::Sound::NR52)};
    byte wave0 {make_byte(Specs::Registers::Sound::WAVE0)};
    byte wave1 {make_byte(Specs::Registers::Sound::WAVE1)};
    byte wave2 {make_byte(Specs::Registers::Sound::WAVE2)};
    byte wave3 {make_byte(Specs::Registers::Sound::WAVE3)};
    byte wave4 {make_byte(Specs::Registers::Sound::WAVE4)};
    byte wave5 {make_byte(Specs::Registers::Sound::WAVE5)};
    byte wave6 {make_byte(Specs::Registers::Sound::WAVE6)};
    byte wave7 {make_byte(Specs::Registers::Sound::WAVE7)};
    byte wave8 {make_byte(Specs::Registers::Sound::WAVE8)};
    byte wave9 {make_byte(Specs::Registers::Sound::WAVE9)};
    byte waveA {make_byte(Specs::Registers::Sound::WAVEA)};
    byte waveB {make_byte(Specs::Registers::Sound::WAVEB)};
    byte waveC {make_byte(Specs::Registers::Sound::WAVEC)};
    byte waveD {make_byte(Specs::Registers::Sound::WAVED)};
    byte waveE {make_byte(Specs::Registers::Sound::WAVEE)};
    byte waveF {make_byte(Specs::Registers::Sound::WAVEF)};
};
#endif // SOUND_H