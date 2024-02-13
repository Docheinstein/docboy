#ifndef SOUND_H
#define SOUND_H

#include "docboy/bootrom/macros.h"
#include "docboy/memory/byte.hpp"

class SoundIO {
public:
    void writeNR10(uint8_t value) {
        NR10 = 0b10000000 | value;
    }

    void writeNR30(uint8_t value) {
        NR30 = 0b01111111 | value;
    }

    void writeNR32(uint8_t value) {
        NR30 = 0b10011111 | value;
    }

    void writeNR41(uint8_t value) {
        NR41 = 0b11000000 | value;
    }

    void writeNR44(uint8_t value) {
        NR44 = 0b00111111 | value;
    }

    void writeNR52(uint8_t value) {
        NR52 = 0b01110000 | value;
    }

    void saveState(Parcel& parcel) const {
        parcel.writeUInt8(NR10);
        parcel.writeUInt8(NR11);
        parcel.writeUInt8(NR12);
        parcel.writeUInt8(NR13);
        parcel.writeUInt8(NR14);
        parcel.writeUInt8(NR21);
        parcel.writeUInt8(NR22);
        parcel.writeUInt8(NR23);
        parcel.writeUInt8(NR24);
        parcel.writeUInt8(NR30);
        parcel.writeUInt8(NR31);
        parcel.writeUInt8(NR32);
        parcel.writeUInt8(NR33);
        parcel.writeUInt8(NR34);
        parcel.writeUInt8(NR41);
        parcel.writeUInt8(NR42);
        parcel.writeUInt8(NR43);
        parcel.writeUInt8(NR44);
        parcel.writeUInt8(NR50);
        parcel.writeUInt8(NR51);
        parcel.writeUInt8(NR52);
        parcel.writeUInt8(WAVE0);
        parcel.writeUInt8(WAVE1);
        parcel.writeUInt8(WAVE2);
        parcel.writeUInt8(WAVE3);
        parcel.writeUInt8(WAVE4);
        parcel.writeUInt8(WAVE5);
        parcel.writeUInt8(WAVE6);
        parcel.writeUInt8(WAVE7);
        parcel.writeUInt8(WAVE8);
        parcel.writeUInt8(WAVE9);
        parcel.writeUInt8(WAVEA);
        parcel.writeUInt8(WAVEB);
        parcel.writeUInt8(WAVEC);
        parcel.writeUInt8(WAVED);
        parcel.writeUInt8(WAVEE);
        parcel.writeUInt8(WAVEF);
    }

    void loadState(Parcel& parcel) {
        NR10 = parcel.readUInt8();
        NR11 = parcel.readUInt8();
        NR12 = parcel.readUInt8();
        NR13 = parcel.readUInt8();
        NR14 = parcel.readUInt8();
        NR21 = parcel.readUInt8();
        NR22 = parcel.readUInt8();
        NR23 = parcel.readUInt8();
        NR24 = parcel.readUInt8();
        NR30 = parcel.readUInt8();
        NR31 = parcel.readUInt8();
        NR32 = parcel.readUInt8();
        NR33 = parcel.readUInt8();
        NR34 = parcel.readUInt8();
        NR41 = parcel.readUInt8();
        NR42 = parcel.readUInt8();
        NR43 = parcel.readUInt8();
        NR44 = parcel.readUInt8();
        NR50 = parcel.readUInt8();
        NR51 = parcel.readUInt8();
        NR52 = parcel.readUInt8();
        WAVE0 = parcel.readUInt8();
        WAVE1 = parcel.readUInt8();
        WAVE2 = parcel.readUInt8();
        WAVE3 = parcel.readUInt8();
        WAVE4 = parcel.readUInt8();
        WAVE5 = parcel.readUInt8();
        WAVE6 = parcel.readUInt8();
        WAVE7 = parcel.readUInt8();
        WAVE8 = parcel.readUInt8();
        WAVE9 = parcel.readUInt8();
        WAVEA = parcel.readUInt8();
        WAVEB = parcel.readUInt8();
        WAVEC = parcel.readUInt8();
        WAVED = parcel.readUInt8();
        WAVEE = parcel.readUInt8();
        WAVEF = parcel.readUInt8();
    }

    byte NR10 {make_byte(Specs::Registers::Sound::NR10, 0b10000000)};
    byte NR11 {make_byte(Specs::Registers::Sound::NR11, IF_BOOTROM_ELSE(0, 0b10111111))};
    byte NR12 {make_byte(Specs::Registers::Sound::NR12, IF_BOOTROM_ELSE(0, 0b11110011))};
    byte NR13 {make_byte(Specs::Registers::Sound::NR13, IF_BOOTROM_ELSE(0, 0b11111111))};
    byte NR14 {make_byte(Specs::Registers::Sound::NR14, 0b10111111)}; // TODO: B8 or BF?
    byte NR21 {make_byte(Specs::Registers::Sound::NR21, IF_BOOTROM_ELSE(0, 0b00111111))};
    byte NR22 {make_byte(Specs::Registers::Sound::NR22)};
    byte NR23 {make_byte(Specs::Registers::Sound::NR23, IF_BOOTROM_ELSE(0, 0b11111111))};
    byte NR24 {make_byte(Specs::Registers::Sound::NR24, 0b10111111)}; // TODO: B8 or BF?
    byte NR30 {make_byte(Specs::Registers::Sound::NR30, 0b01111111)};
    byte NR31 {make_byte(Specs::Registers::Sound::NR31, IF_BOOTROM_ELSE(0, 0b11111111))};
    byte NR32 {make_byte(Specs::Registers::Sound::NR32, 0b10011111)};
    byte NR33 {make_byte(Specs::Registers::Sound::NR33, IF_BOOTROM_ELSE(0, 0b11111111))};
    byte NR34 {make_byte(Specs::Registers::Sound::NR34, 0b10111111)}; // TODO: B8 or BF?
    byte NR41 {make_byte(Specs::Registers::Sound::NR41, IF_BOOTROM_ELSE(0, 0b11111111))};
    byte NR42 {make_byte(Specs::Registers::Sound::NR42)};
    byte NR43 {make_byte(Specs::Registers::Sound::NR43)};
    byte NR44 {make_byte(Specs::Registers::Sound::NR44, 0b10111111)}; // TODO: B8 or BF?
    byte NR50 {make_byte(Specs::Registers::Sound::NR50, IF_BOOTROM_ELSE(0, 0b01110111))};
    byte NR51 {make_byte(Specs::Registers::Sound::NR51, IF_BOOTROM_ELSE(0, 0b11110011))};
    byte NR52 {make_byte(Specs::Registers::Sound::NR52, IF_BOOTROM_ELSE(0, 0b11110001))};
    byte WAVE0 {make_byte(Specs::Registers::Sound::WAVE0)};
    byte WAVE1 {make_byte(Specs::Registers::Sound::WAVE1)};
    byte WAVE2 {make_byte(Specs::Registers::Sound::WAVE2)};
    byte WAVE3 {make_byte(Specs::Registers::Sound::WAVE3)};
    byte WAVE4 {make_byte(Specs::Registers::Sound::WAVE4)};
    byte WAVE5 {make_byte(Specs::Registers::Sound::WAVE5)};
    byte WAVE6 {make_byte(Specs::Registers::Sound::WAVE6)};
    byte WAVE7 {make_byte(Specs::Registers::Sound::WAVE7)};
    byte WAVE8 {make_byte(Specs::Registers::Sound::WAVE8)};
    byte WAVE9 {make_byte(Specs::Registers::Sound::WAVE9)};
    byte WAVEA {make_byte(Specs::Registers::Sound::WAVEA)};
    byte WAVEB {make_byte(Specs::Registers::Sound::WAVEB)};
    byte WAVEC {make_byte(Specs::Registers::Sound::WAVEC)};
    byte WAVED {make_byte(Specs::Registers::Sound::WAVED)};
    byte WAVEE {make_byte(Specs::Registers::Sound::WAVEE)};
    byte WAVEF {make_byte(Specs::Registers::Sound::WAVEF)};
};
#endif // SOUND_H