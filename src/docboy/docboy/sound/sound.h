#ifndef SOUND_H
#define SOUND_H

#include "docboy/memory/cell.h"

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

    UInt8 nr10 {make_uint8(Specs::Registers::Sound::NR10)};
    UInt8 nr11 {make_uint8(Specs::Registers::Sound::NR11)};
    UInt8 nr12 {make_uint8(Specs::Registers::Sound::NR12)};
    UInt8 nr13 {make_uint8(Specs::Registers::Sound::NR13)};
    UInt8 nr14 {make_uint8(Specs::Registers::Sound::NR14)};
    UInt8 nr21 {make_uint8(Specs::Registers::Sound::NR21)};
    UInt8 nr22 {make_uint8(Specs::Registers::Sound::NR22)};
    UInt8 nr23 {make_uint8(Specs::Registers::Sound::NR23)};
    UInt8 nr24 {make_uint8(Specs::Registers::Sound::NR24)};
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
    UInt8 nr52 {make_uint8(Specs::Registers::Sound::NR52)};
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
};
#endif // SOUND_H