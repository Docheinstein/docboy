#include "core.h"
#include "core/debugger/gameboy.h"
#include "core/definitions.h"
#include "core/state/state.h"
#include <iostream>

void DebuggableCore::MemoryObserver::onRead(uint16_t addr, uint8_t value) {
    observer.onRead(base + addr, value);
}

void DebuggableCore::MemoryObserver::onReadError(uint16_t addr, const std::string& error) {
    observer.onReadError(base + addr, error);
}

void DebuggableCore::MemoryObserver::onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(base + addr, oldValue, newValue);
}

void DebuggableCore::MemoryObserver::onWriteError(uint16_t addr, const std::string& error) {
    observer.onWriteError(base + addr, error);
}

DebuggableCore::MemoryObserver::MemoryObserver(IMemoryDebug::Observer& observer, uint16_t base) :
    observer(observer),
    base(base) {
}

DebuggableCore::DebuggableCore(IDebuggableGameBoy& gameboy) :
    Core(gameboy),
    observer(),
    bootRomObserver(*this, MemoryMap::ROM0::START),
    cartridgeObserver(*this, MemoryMap::ROM0::START),
    vramObserver(*this, MemoryMap::VRAM::START),
    wram1Observer(*this, MemoryMap::WRAM1::START),
    wram2Observer(*this, MemoryMap::WRAM2::START),
    oamObserver(*this, MemoryMap::OAM::START),
    hramObserver(*this, MemoryMap::HRAM::START),
    ioObserver(*this),
    on(true) {
    gameboy.getCartridgeSlotDebug().setObserver(&cartridgeObserver);
    gameboy.getBootROMDebug().setObserver(&bootRomObserver);

    gameboy.getVRAMDebug().setObserver(&vramObserver);
    gameboy.getWRAM1Debug().setObserver(&wram1Observer);
    gameboy.getWRAM2Debug().setObserver(&wram2Observer);
    gameboy.getOAMDebug().setObserver(&oamObserver);
    gameboy.getHRAMDebug().setObserver(&hramObserver);

    gameboy.getInterruptsIODebug().setObserver(&ioObserver);
    gameboy.getJoypadIODebug().setObserver(&ioObserver);
    gameboy.getLCDIODebug().setObserver(&ioObserver);
    gameboy.getSerialIODebug().setObserver(&ioObserver);
    gameboy.getSoundIODebug().setObserver(&ioObserver);
    gameboy.getTimersIODebug().setObserver(&ioObserver);
    gameboy.getBootIODebug().setObserver(&ioObserver);
}

void DebuggableCore::onRead(uint16_t addr, uint8_t value) {
    if (observer)
        observer->onMemoryRead(addr, value);
}

void DebuggableCore::onReadError(uint16_t addr, const std::string& error) {
    if (observer)
        observer->onMemoryReadError(addr, error);
}

void DebuggableCore::onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) {
    if (observer)
        observer->onMemoryWrite(addr, oldValue, newValue);
}

void DebuggableCore::onWriteError(uint16_t addr, const std::string& error) {
    if (observer)
        observer->onMemoryWriteError(addr, error);
}

void DebuggableCore::tick() {
    if (observer) {
        if (!observer->onTick(gameboy.getClock().getTicks()))
            on = false;
    }
    return Core::tick();
}

void DebuggableCore::setObserver(ICoreDebug::Observer* o) {
    observer = o;
}

ICoreDebug::Observer* DebuggableCore::getObserver() {
    return observer;
}

bool DebuggableCore::isOn() {
    return on;
}

IDebuggableGameBoy& DebuggableCore::getGameBoy() {
    return dynamic_cast<IDebuggableGameBoy&>(gameboy);
}

DebuggableCore::IOObserver::IOObserver(IMemoryDebug::Observer& observer) :
    observer(observer) {
}

void DebuggableCore::IOObserver::onReadP1(uint8_t value) {
    observer.onRead(Registers::Joypad::P1, value);
}

void DebuggableCore::IOObserver::onWriteP1(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Joypad::P1, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadSB(uint8_t value) {
    observer.onRead(Registers::Serial::SB, value);
}

void DebuggableCore::IOObserver::onWriteSB(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Serial::SB, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadSC(uint8_t value) {
    observer.onRead(Registers::Serial::SC, value);
}

void DebuggableCore::IOObserver::onWriteSC(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Serial::SC, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadDIV(uint8_t value) {
    observer.onRead(Registers::Timers::DIV, value);
}

void DebuggableCore::IOObserver::onWriteDIV(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Timers::DIV, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadTIMA(uint8_t value) {
    observer.onRead(Registers::Timers::TIMA, value);
}

void DebuggableCore::IOObserver::onWriteTIMA(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Timers::TIMA, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadTMA(uint8_t value) {
    observer.onRead(Registers::Timers::TMA, value);
}

void DebuggableCore::IOObserver::onWriteTMA(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Timers::TMA, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadTAC(uint8_t value) {
    observer.onRead(Registers::Timers::TAC, value);
}

void DebuggableCore::IOObserver::onWriteTAC(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Timers::TAC, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadIF(uint8_t value) {
    observer.onRead(Registers::Interrupts::IF, value);
}

void DebuggableCore::IOObserver::onWriteIF(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Interrupts::IF, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadIE(uint8_t value) {
    observer.onRead(Registers::Interrupts::IE, value);
}

void DebuggableCore::IOObserver::onWriteIE(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Interrupts::IE, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR10(uint8_t value) {
    observer.onRead(Registers::Sound::NR10, value);
}

void DebuggableCore::IOObserver::onWriteNR10(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR10, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR11(uint8_t value) {
    observer.onRead(Registers::Sound::NR11, value);
}

void DebuggableCore::IOObserver::onWriteNR11(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR11, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR12(uint8_t value) {
    observer.onRead(Registers::Sound::NR12, value);
}

void DebuggableCore::IOObserver::onWriteNR12(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR12, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR13(uint8_t value) {
    observer.onRead(Registers::Sound::NR13, value);
}

void DebuggableCore::IOObserver::onWriteNR13(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR13, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR14(uint8_t value) {
    observer.onRead(Registers::Sound::NR14, value);
}

void DebuggableCore::IOObserver::onWriteNR14(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR14, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR21(uint8_t value) {
    observer.onRead(Registers::Sound::NR21, value);
}

void DebuggableCore::IOObserver::onWriteNR21(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR21, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR22(uint8_t value) {
    observer.onRead(Registers::Sound::NR22, value);
}

void DebuggableCore::IOObserver::onWriteNR22(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR22, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR23(uint8_t value) {
    observer.onRead(Registers::Sound::NR23, value);
}

void DebuggableCore::IOObserver::onWriteNR23(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR23, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR24(uint8_t value) {
    observer.onRead(Registers::Sound::NR24, value);
}

void DebuggableCore::IOObserver::onWriteNR24(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR24, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR30(uint8_t value) {
    observer.onRead(Registers::Sound::NR30, value);
}

void DebuggableCore::IOObserver::onWriteNR30(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR30, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR31(uint8_t value) {
    observer.onRead(Registers::Sound::NR31, value);
}

void DebuggableCore::IOObserver::onWriteNR31(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR31, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR32(uint8_t value) {
    observer.onRead(Registers::Sound::NR32, value);
}

void DebuggableCore::IOObserver::onWriteNR32(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR32, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR33(uint8_t value) {
    observer.onRead(Registers::Sound::NR33, value);
}

void DebuggableCore::IOObserver::onWriteNR33(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR33, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR34(uint8_t value) {
    observer.onRead(Registers::Sound::NR34, value);
}

void DebuggableCore::IOObserver::onWriteNR34(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR34, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR41(uint8_t value) {
    observer.onRead(Registers::Sound::NR41, value);
}

void DebuggableCore::IOObserver::onWriteNR41(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR41, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR42(uint8_t value) {
    observer.onRead(Registers::Sound::NR42, value);
}

void DebuggableCore::IOObserver::onWriteNR42(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR42, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR43(uint8_t value) {
    observer.onRead(Registers::Sound::NR43, value);
}

void DebuggableCore::IOObserver::onWriteNR43(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR43, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR44(uint8_t value) {
    observer.onRead(Registers::Sound::NR44, value);
}

void DebuggableCore::IOObserver::onWriteNR44(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR44, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR50(uint8_t value) {
    observer.onRead(Registers::Sound::NR50, value);
}

void DebuggableCore::IOObserver::onWriteNR50(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR50, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR51(uint8_t value) {
    observer.onRead(Registers::Sound::NR51, value);
}

void DebuggableCore::IOObserver::onWriteNR51(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR51, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadNR52(uint8_t value) {
    observer.onRead(Registers::Sound::NR52, value);
}

void DebuggableCore::IOObserver::onWriteNR52(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::NR52, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVE0(uint8_t value) {
    observer.onRead(Registers::Sound::WAVE0, value);
}

void DebuggableCore::IOObserver::onWriteWAVE0(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVE0, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVE1(uint8_t value) {
    observer.onRead(Registers::Sound::WAVE1, value);
}

void DebuggableCore::IOObserver::onWriteWAVE1(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVE1, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVE2(uint8_t value) {
    observer.onRead(Registers::Sound::WAVE2, value);
}

void DebuggableCore::IOObserver::onWriteWAVE2(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVE2, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVE3(uint8_t value) {
    observer.onRead(Registers::Sound::WAVE3, value);
}

void DebuggableCore::IOObserver::onWriteWAVE3(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVE3, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVE4(uint8_t value) {
    observer.onRead(Registers::Sound::WAVE4, value);
}

void DebuggableCore::IOObserver::onWriteWAVE4(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVE4, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVE5(uint8_t value) {
    observer.onRead(Registers::Sound::WAVE5, value);
}

void DebuggableCore::IOObserver::onWriteWAVE5(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVE5, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVE6(uint8_t value) {
    observer.onRead(Registers::Sound::WAVE6, value);
}

void DebuggableCore::IOObserver::onWriteWAVE6(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVE6, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVE7(uint8_t value) {
    observer.onRead(Registers::Sound::WAVE7, value);
}

void DebuggableCore::IOObserver::onWriteWAVE7(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVE7, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVE8(uint8_t value) {
    observer.onRead(Registers::Sound::WAVE8, value);
}

void DebuggableCore::IOObserver::onWriteWAVE8(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVE8, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVE9(uint8_t value) {
    observer.onRead(Registers::Sound::WAVE9, value);
}

void DebuggableCore::IOObserver::onWriteWAVE9(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVE9, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVEA(uint8_t value) {
    observer.onRead(Registers::Sound::WAVEA, value);
}

void DebuggableCore::IOObserver::onWriteWAVEA(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVEA, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVEB(uint8_t value) {
    observer.onRead(Registers::Sound::WAVEB, value);
}

void DebuggableCore::IOObserver::onWriteWAVEB(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVEB, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVEC(uint8_t value) {
    observer.onRead(Registers::Sound::WAVEC, value);
}

void DebuggableCore::IOObserver::onWriteWAVEC(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVEC, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVED(uint8_t value) {
    observer.onRead(Registers::Sound::WAVED, value);
}

void DebuggableCore::IOObserver::onWriteWAVED(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVED, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVEE(uint8_t value) {
    observer.onRead(Registers::Sound::WAVEE, value);
}

void DebuggableCore::IOObserver::onWriteWAVEE(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVEE, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWAVEF(uint8_t value) {
    observer.onRead(Registers::Sound::WAVEF, value);
}

void DebuggableCore::IOObserver::onWriteWAVEF(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Sound::WAVEF, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadLCDC(uint8_t value) {
    observer.onRead(Registers::LCD::LCDC, value);
}

void DebuggableCore::IOObserver::onWriteLCDC(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::LCDC, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadSTAT(uint8_t value) {
    observer.onRead(Registers::LCD::STAT, value);
}

void DebuggableCore::IOObserver::onWriteSTAT(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::STAT, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadSCY(uint8_t value) {
    observer.onRead(Registers::LCD::SCY, value);
}

void DebuggableCore::IOObserver::onWriteSCY(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::SCY, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadSCX(uint8_t value) {
    observer.onRead(Registers::LCD::SCX, value);
}

void DebuggableCore::IOObserver::onWriteSCX(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::SCX, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadLY(uint8_t value) {
    observer.onRead(Registers::LCD::LY, value);
}

void DebuggableCore::IOObserver::onWriteLY(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::LY, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadLYC(uint8_t value) {
    observer.onRead(Registers::LCD::LYC, value);
}

void DebuggableCore::IOObserver::onWriteLYC(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::LYC, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadDMA(uint8_t value) {
    observer.onRead(Registers::LCD::DMA, value);
}

void DebuggableCore::IOObserver::onWriteDMA(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::DMA, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadBGP(uint8_t value) {
    observer.onRead(Registers::LCD::BGP, value);
}

void DebuggableCore::IOObserver::onWriteBGP(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::BGP, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadOBP0(uint8_t value) {
    observer.onRead(Registers::LCD::OBP0, value);
}

void DebuggableCore::IOObserver::onWriteOBP0(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::OBP0, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadOBP1(uint8_t value) {
    observer.onRead(Registers::LCD::OBP1, value);
}

void DebuggableCore::IOObserver::onWriteOBP1(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::OBP1, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWY(uint8_t value) {
    observer.onRead(Registers::LCD::WY, value);
}

void DebuggableCore::IOObserver::onWriteWY(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::WY, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadWX(uint8_t value) {
    observer.onRead(Registers::LCD::WX, value);
}

void DebuggableCore::IOObserver::onWriteWX(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::LCD::WX, oldValue, newValue);
}

void DebuggableCore::IOObserver::onReadBOOT(uint8_t value) {
    observer.onRead(Registers::Boot::BOOT, value);
}

void DebuggableCore::IOObserver::onWriteBOOT(uint8_t oldValue, uint8_t newValue) {
    observer.onWrite(Registers::Boot::BOOT, oldValue, newValue);
}
