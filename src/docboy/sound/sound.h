#ifndef SOUND_H
#define SOUND_H

#include "docboy/memory/byte.hpp"

class SoundIO {
public:
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

    BYTE(NR10, Specs::Registers::Sound::NR10);
    BYTE(NR11, Specs::Registers::Sound::NR11);
    BYTE(NR12, Specs::Registers::Sound::NR12);
    BYTE(NR13, Specs::Registers::Sound::NR13);
    BYTE(NR14, Specs::Registers::Sound::NR14);
    BYTE(NR21, Specs::Registers::Sound::NR21);
    BYTE(NR22, Specs::Registers::Sound::NR22);
    BYTE(NR23, Specs::Registers::Sound::NR23);
    BYTE(NR24, Specs::Registers::Sound::NR24);
    BYTE(NR30, Specs::Registers::Sound::NR30);
    BYTE(NR31, Specs::Registers::Sound::NR31);
    BYTE(NR32, Specs::Registers::Sound::NR32);
    BYTE(NR33, Specs::Registers::Sound::NR33);
    BYTE(NR34, Specs::Registers::Sound::NR34);
    BYTE(NR41, Specs::Registers::Sound::NR41);
    BYTE(NR42, Specs::Registers::Sound::NR42);
    BYTE(NR43, Specs::Registers::Sound::NR43);
    BYTE(NR44, Specs::Registers::Sound::NR44);
    BYTE(NR50, Specs::Registers::Sound::NR50);
    BYTE(NR51, Specs::Registers::Sound::NR51);
    BYTE(NR52, Specs::Registers::Sound::NR52);
    BYTE(WAVE0, Specs::Registers::Sound::WAVE0);
    BYTE(WAVE1, Specs::Registers::Sound::WAVE1);
    BYTE(WAVE2, Specs::Registers::Sound::WAVE2);
    BYTE(WAVE3, Specs::Registers::Sound::WAVE3);
    BYTE(WAVE4, Specs::Registers::Sound::WAVE4);
    BYTE(WAVE5, Specs::Registers::Sound::WAVE5);
    BYTE(WAVE6, Specs::Registers::Sound::WAVE6);
    BYTE(WAVE7, Specs::Registers::Sound::WAVE7);
    BYTE(WAVE8, Specs::Registers::Sound::WAVE8);
    BYTE(WAVE9, Specs::Registers::Sound::WAVE9);
    BYTE(WAVEA, Specs::Registers::Sound::WAVEA);
    BYTE(WAVEB, Specs::Registers::Sound::WAVEB);
    BYTE(WAVEC, Specs::Registers::Sound::WAVEC);
    BYTE(WAVED, Specs::Registers::Sound::WAVED);
    BYTE(WAVEE, Specs::Registers::Sound::WAVEE);
    BYTE(WAVEF, Specs::Registers::Sound::WAVEF);
};
#endif // SOUND_H