#include "sound.h"

DebuggableSound::DebuggableSound() :
    observer() {
}

uint8_t DebuggableSound::readNR10() const {
    uint8_t value = Sound::readNR10();
    if (observer)
        observer->onReadNR10(value);
    return value;
}

void DebuggableSound::writeNR10(uint8_t value) {
    uint8_t oldValue = Sound::readNR10();
    Sound::writeNR10(value);
    if (observer)
        observer->onWriteNR10(oldValue, value);
}

uint8_t DebuggableSound::readNR11() const {
    uint8_t value = Sound::readNR11();
    if (observer)
        observer->onReadNR11(value);
    return value;
}

void DebuggableSound::writeNR11(uint8_t value) {
    uint8_t oldValue = Sound::readNR11();
    Sound::writeNR11(value);
    if (observer)
        observer->onWriteNR11(oldValue, value);
}

uint8_t DebuggableSound::readNR12() const {
    uint8_t value = Sound::readNR12();
    if (observer)
        observer->onReadNR12(value);
    return value;
}

void DebuggableSound::writeNR12(uint8_t value) {
    uint8_t oldValue = Sound::readNR12();
    Sound::writeNR12(value);
    if (observer)
        observer->onWriteNR12(oldValue, value);
}

uint8_t DebuggableSound::readNR13() const {
    uint8_t value = Sound::readNR13();
    if (observer)
        observer->onReadNR13(value);
    return value;
}

void DebuggableSound::writeNR13(uint8_t value) {
    uint8_t oldValue = Sound::readNR13();
    Sound::writeNR13(value);
    if (observer)
        observer->onWriteNR13(oldValue, value);
}

uint8_t DebuggableSound::readNR14() const {
    uint8_t value = Sound::readNR14();
    if (observer)
        observer->onReadNR14(value);
    return value;
}

void DebuggableSound::writeNR14(uint8_t value) {
    uint8_t oldValue = Sound::readNR14();
    Sound::writeNR14(value);
    if (observer)
        observer->onWriteNR14(oldValue, value);
}

uint8_t DebuggableSound::readNR21() const {
    uint8_t value = Sound::readNR21();
    if (observer)
        observer->onReadNR21(value);
    return value;
}

void DebuggableSound::writeNR21(uint8_t value) {
    uint8_t oldValue = Sound::readNR21();
    Sound::writeNR21(value);
    if (observer)
        observer->onWriteNR21(oldValue, value);
}

uint8_t DebuggableSound::readNR22() const {
    uint8_t value = Sound::readNR22();
    if (observer)
        observer->onReadNR22(value);
    return value;
}

void DebuggableSound::writeNR22(uint8_t value) {
    uint8_t oldValue = Sound::readNR22();
    Sound::writeNR22(value);
    if (observer)
        observer->onWriteNR22(oldValue, value);
}

uint8_t DebuggableSound::readNR23() const {
    uint8_t value = Sound::readNR23();
    if (observer)
        observer->onReadNR23(value);
    return value;
}

void DebuggableSound::writeNR23(uint8_t value) {
    uint8_t oldValue = Sound::readNR23();
    Sound::writeNR23(value);
    if (observer)
        observer->onWriteNR23(oldValue, value);
}

uint8_t DebuggableSound::readNR24() const {
    uint8_t value = Sound::readNR24();
    if (observer)
        observer->onReadNR24(value);
    return value;
}

void DebuggableSound::writeNR24(uint8_t value) {
    uint8_t oldValue = Sound::readNR24();
    Sound::writeNR24(value);
    if (observer)
        observer->onWriteNR24(oldValue, value);
}

uint8_t DebuggableSound::readNR30() const {
    uint8_t value = Sound::readNR30();
    if (observer)
        observer->onReadNR30(value);
    return value;
}

void DebuggableSound::writeNR30(uint8_t value) {
    uint8_t oldValue = Sound::readNR30();
    Sound::writeNR30(value);
    if (observer)
        observer->onWriteNR30(oldValue, value);
}

uint8_t DebuggableSound::readNR31() const {
    uint8_t value = Sound::readNR31();
    if (observer)
        observer->onReadNR31(value);
    return value;
}

void DebuggableSound::writeNR31(uint8_t value) {
    uint8_t oldValue = Sound::readNR31();
    Sound::writeNR31(value);
    if (observer)
        observer->onWriteNR31(oldValue, value);
}

uint8_t DebuggableSound::readNR32() const {
    uint8_t value = Sound::readNR32();
    if (observer)
        observer->onReadNR32(value);
    return value;
}

void DebuggableSound::writeNR32(uint8_t value) {
    uint8_t oldValue = Sound::readNR32();
    Sound::writeNR32(value);
    if (observer)
        observer->onWriteNR32(oldValue, value);
}

uint8_t DebuggableSound::readNR33() const {
    uint8_t value = Sound::readNR33();
    if (observer)
        observer->onReadNR33(value);
    return value;
}

void DebuggableSound::writeNR33(uint8_t value) {
    uint8_t oldValue = Sound::readNR33();
    Sound::writeNR33(value);
    if (observer)
        observer->onWriteNR33(oldValue, value);
}

uint8_t DebuggableSound::readNR34() const {
    uint8_t value = Sound::readNR34();
    if (observer)
        observer->onReadNR34(value);
    return value;
}

void DebuggableSound::writeNR34(uint8_t value) {
    uint8_t oldValue = Sound::readNR34();
    Sound::writeNR34(value);
    if (observer)
        observer->onWriteNR34(oldValue, value);
}

uint8_t DebuggableSound::readNR41() const {
    uint8_t value = Sound::readNR41();
    if (observer)
        observer->onReadNR41(value);
    return value;
}

void DebuggableSound::writeNR41(uint8_t value) {
    uint8_t oldValue = Sound::readNR41();
    Sound::writeNR41(value);
    if (observer)
        observer->onWriteNR41(oldValue, value);
}

uint8_t DebuggableSound::readNR42() const {
    uint8_t value = Sound::readNR42();
    if (observer)
        observer->onReadNR42(value);
    return value;
}

void DebuggableSound::writeNR42(uint8_t value) {
    uint8_t oldValue = Sound::readNR42();
    Sound::writeNR42(value);
    if (observer)
        observer->onWriteNR42(oldValue, value);
}

uint8_t DebuggableSound::readNR43() const {
    uint8_t value = Sound::readNR43();
    if (observer)
        observer->onReadNR43(value);
    return value;
}

void DebuggableSound::writeNR43(uint8_t value) {
    uint8_t oldValue = Sound::readNR43();
    Sound::writeNR43(value);
    if (observer)
        observer->onWriteNR43(oldValue, value);
}

uint8_t DebuggableSound::readNR44() const {
    uint8_t value = Sound::readNR44();
    if (observer)
        observer->onReadNR44(value);
    return value;
}

void DebuggableSound::writeNR44(uint8_t value) {
    uint8_t oldValue = Sound::readNR44();
    Sound::writeNR44(value);
    if (observer)
        observer->onWriteNR44(oldValue, value);
}

uint8_t DebuggableSound::readNR50() const {
    uint8_t value = Sound::readNR50();
    if (observer)
        observer->onReadNR50(value);
    return value;
}

void DebuggableSound::writeNR50(uint8_t value) {
    uint8_t oldValue = Sound::readNR50();
    Sound::writeNR50(value);
    if (observer)
        observer->onWriteNR50(oldValue, value);
}

uint8_t DebuggableSound::readNR51() const {
    uint8_t value = Sound::readNR51();
    if (observer)
        observer->onReadNR51(value);
    return value;
}

void DebuggableSound::writeNR51(uint8_t value) {
    uint8_t oldValue = Sound::readNR51();
    Sound::writeNR51(value);
    if (observer)
        observer->onWriteNR51(oldValue, value);
}

uint8_t DebuggableSound::readNR52() const {
    uint8_t value = Sound::readNR52();
    if (observer)
        observer->onReadNR52(value);
    return value;
}

void DebuggableSound::writeNR52(uint8_t value) {
    uint8_t oldValue = Sound::readNR52();
    Sound::writeNR52(value);
    if (observer)
        observer->onWriteNR52(oldValue, value);
}

uint8_t DebuggableSound::readWAVE0() const {
    uint8_t value = Sound::readWAVE0();
    if (observer)
        observer->onReadWAVE0(value);
    return value;
}

void DebuggableSound::writeWAVE0(uint8_t value) {
    uint8_t oldValue = Sound::readWAVE0();
    Sound::writeWAVE0(value);
    if (observer)
        observer->onWriteWAVE0(oldValue, value);
}

uint8_t DebuggableSound::readWAVE1() const {
    uint8_t value = Sound::readWAVE1();
    if (observer)
        observer->onReadWAVE1(value);
    return value;
}

void DebuggableSound::writeWAVE1(uint8_t value) {
    uint8_t oldValue = Sound::readWAVE1();
    Sound::writeWAVE1(value);
    if (observer)
        observer->onWriteWAVE1(oldValue, value);
}

uint8_t DebuggableSound::readWAVE2() const {
    uint8_t value = Sound::readWAVE2();
    if (observer)
        observer->onReadWAVE2(value);
    return value;
}

void DebuggableSound::writeWAVE2(uint8_t value) {
    uint8_t oldValue = Sound::readWAVE2();
    Sound::writeWAVE2(value);
    if (observer)
        observer->onWriteWAVE2(oldValue, value);
}

uint8_t DebuggableSound::readWAVE3() const {
    uint8_t value = Sound::readWAVE3();
    if (observer)
        observer->onReadWAVE3(value);
    return value;
}

void DebuggableSound::writeWAVE3(uint8_t value) {
    uint8_t oldValue = Sound::readWAVE3();
    Sound::writeWAVE3(value);
    if (observer)
        observer->onWriteWAVE3(oldValue, value);
}

uint8_t DebuggableSound::readWAVE4() const {
    uint8_t value = Sound::readWAVE4();
    if (observer)
        observer->onReadWAVE4(value);
    return value;
}

void DebuggableSound::writeWAVE4(uint8_t value) {
    uint8_t oldValue = Sound::readWAVE4();
    Sound::writeWAVE4(value);
    if (observer)
        observer->onWriteWAVE4(oldValue, value);
}

uint8_t DebuggableSound::readWAVE5() const {
    uint8_t value = Sound::readWAVE5();
    if (observer)
        observer->onReadWAVE5(value);
    return value;
}

void DebuggableSound::writeWAVE5(uint8_t value) {
    uint8_t oldValue = Sound::readWAVE5();
    Sound::writeWAVE5(value);
    if (observer)
        observer->onWriteWAVE5(oldValue, value);
}

uint8_t DebuggableSound::readWAVE6() const {
    uint8_t value = Sound::readWAVE6();
    if (observer)
        observer->onReadWAVE6(value);
    return value;
}

void DebuggableSound::writeWAVE6(uint8_t value) {
    uint8_t oldValue = Sound::readWAVE6();
    Sound::writeWAVE6(value);
    if (observer)
        observer->onWriteWAVE6(oldValue, value);
}

uint8_t DebuggableSound::readWAVE7() const {
    uint8_t value = Sound::readWAVE7();
    if (observer)
        observer->onReadWAVE7(value);
    return value;
}

void DebuggableSound::writeWAVE7(uint8_t value) {
    uint8_t oldValue = Sound::readWAVE7();
    Sound::writeWAVE7(value);
    if (observer)
        observer->onWriteWAVE7(oldValue, value);
}

uint8_t DebuggableSound::readWAVE8() const {
    uint8_t value = Sound::readWAVE8();
    if (observer)
        observer->onReadWAVE8(value);
    return value;
}

void DebuggableSound::writeWAVE8(uint8_t value) {
    uint8_t oldValue = Sound::readWAVE8();
    Sound::writeWAVE8(value);
    if (observer)
        observer->onWriteWAVE8(oldValue, value);
}

uint8_t DebuggableSound::readWAVE9() const {
    uint8_t value = Sound::readWAVE9();
    if (observer)
        observer->onReadWAVE9(value);
    return value;
}

void DebuggableSound::writeWAVE9(uint8_t value) {
    uint8_t oldValue = Sound::readWAVE9();
    Sound::writeWAVE9(value);
    if (observer)
        observer->onWriteWAVE9(oldValue, value);
}

uint8_t DebuggableSound::readWAVEA() const {
    uint8_t value = Sound::readWAVEA();
    if (observer)
        observer->onReadWAVEA(value);
    return value;
}

void DebuggableSound::writeWAVEA(uint8_t value) {
    uint8_t oldValue = Sound::readWAVEA();
    Sound::writeWAVEA(value);
    if (observer)
        observer->onWriteWAVEA(oldValue, value);
}

uint8_t DebuggableSound::readWAVEB() const {
    uint8_t value = Sound::readWAVEB();
    if (observer)
        observer->onReadWAVEB(value);
    return value;
}

void DebuggableSound::writeWAVEB(uint8_t value) {
    uint8_t oldValue = Sound::readWAVEB();
    Sound::writeWAVEB(value);
    if (observer)
        observer->onWriteWAVEB(oldValue, value);
}

uint8_t DebuggableSound::readWAVEC() const {
    uint8_t value = Sound::readWAVEC();
    if (observer)
        observer->onReadWAVEC(value);
    return value;
}

void DebuggableSound::writeWAVEC(uint8_t value) {
    uint8_t oldValue = Sound::readWAVEC();
    Sound::writeWAVEC(value);
    if (observer)
        observer->onWriteWAVEC(oldValue, value);
}

uint8_t DebuggableSound::readWAVED() const {
    uint8_t value = Sound::readWAVED();
    if (observer)
        observer->onReadWAVED(value);
    return value;
}

void DebuggableSound::writeWAVED(uint8_t value) {
    uint8_t oldValue = Sound::readWAVED();
    Sound::writeWAVED(value);
    if (observer)
        observer->onWriteWAVED(oldValue, value);
}

uint8_t DebuggableSound::readWAVEE() const {
    uint8_t value = Sound::readWAVEE();
    if (observer)
        observer->onReadWAVEE(value);
    return value;
}

void DebuggableSound::writeWAVEE(uint8_t value) {
    uint8_t oldValue = Sound::readWAVEE();
    Sound::writeWAVEE(value);
    if (observer)
        observer->onWriteWAVEE(oldValue, value);
}

uint8_t DebuggableSound::readWAVEF() const {
    uint8_t value = Sound::readWAVEF();
    if (observer)
        observer->onReadWAVEF(value);
    return value;
}

void DebuggableSound::writeWAVEF(uint8_t value) {
    uint8_t oldValue = Sound::readWAVEF();
    Sound::writeWAVEF(value);
    if (observer)
        observer->onWriteWAVEF(oldValue, value);
}

void DebuggableSound::setObserver(Observer* o) {
    observer = o;
}
