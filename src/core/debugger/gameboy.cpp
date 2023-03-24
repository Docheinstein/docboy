#include "gameboy.h"

DebuggableGameBoy::Builder::Builder(): bootRom(), frequency() {

}

DebuggableGameBoy::Builder &DebuggableGameBoy::Builder::setBootROM(std::unique_ptr<IBootROM> bootRom_) {
    bootRom = std::move(bootRom_);
    return *this;
}

DebuggableGameBoy::Builder &DebuggableGameBoy::Builder::setFrequency(uint64_t frequency_) {
    frequency = frequency_;
    return *this;
}

DebuggableGameBoy DebuggableGameBoy::Builder::build() {
    return DebuggableGameBoy(std::move(bootRom), frequency);
}

DebuggableGameBoy::DebuggableGameBoy(std::unique_ptr<IBootROM> bootRom_, uint64_t frequency) :
        vram(MemoryMap::VRAM::SIZE),
        wram1(MemoryMap::WRAM1::SIZE),
        wram2(MemoryMap::WRAM2::SIZE),
        oam(MemoryMap::OAM::SIZE),
        hram(MemoryMap::HRAM::SIZE),
        interrupts(),
        joypad(interrupts),
        serialPort(interrupts),
        timers(interrupts),
        sound(),
        boot(std::move(bootRom_)),
        bus(vram, wram1, wram2, oam, hram, cartridgeSlot, boot,
            joypad, serialPort, timers, interrupts, sound, lcd,
            boot),
        cpu(bus, timers, serialPort, boot.readBOOT() == 0),
        ppu(lcd, lcd, interrupts, vram, oam),
        clock() {
    uint64_t ppuFreq = frequency != 0 ? frequency : Specs::PPU::FREQUENCY;
    uint64_t cpuFreq = ppuFreq / (Specs::PPU::FREQUENCY / Specs::CPU::FREQUENCY);
    clock.attach(&cpu, cpuFreq);
    clock.attach(&ppu, ppuFreq);
}

IMemory &DebuggableGameBoy::getVRAM() {
    return vram;
}

IMemory &DebuggableGameBoy::getWRAM1() {
    return wram1;
}

IMemory &DebuggableGameBoy::getWRAM2() {
    return wram2;
}

IMemory &DebuggableGameBoy::getOAM() {
    return oam;
}

IMemory &DebuggableGameBoy::getHRAM() {
    return hram;
}

IBootIO &DebuggableGameBoy::getBootIO() {
    return boot;
}

IInterruptsIO &DebuggableGameBoy::getInterruptsIO() {
    return interrupts;
}

IJoypadIO &DebuggableGameBoy::getJoypadIO() {
    return joypad;
}

ILCDIO &DebuggableGameBoy::getLCDIO() {
    return lcd;
}

ISerialIO &DebuggableGameBoy::getSerialIO() {
    return serialPort;
}

ISoundIO &DebuggableGameBoy::getSoundIO() {
    return sound;
}

ITimersIO &DebuggableGameBoy::getTimersIO() {
    return timers;
}

ICPU &DebuggableGameBoy::getCPU() {
    return cpu;
}

IPPU &DebuggableGameBoy::getPPU() {
    return ppu;
}

IClock &DebuggableGameBoy::getClock() {
    return clock;
}

IJoypad &DebuggableGameBoy::getJoypad() {
    return joypad;
}

ISerialPort &DebuggableGameBoy::getSerialPort() {
    return serialPort;
}

ICartridgeSlot &DebuggableGameBoy::getCartridgeSlot() {
    return cartridgeSlot;
}

IBus &DebuggableGameBoy::getBus() {
    return bus;
}

IMemoryDebug &DebuggableGameBoy::getVRAMDebug() {
    return vram;
}

IMemoryDebug &DebuggableGameBoy::getWRAM1Debug() {
    return wram1;
}

IMemoryDebug &DebuggableGameBoy::getWRAM2Debug() {
    return wram2;
}

IMemoryDebug &DebuggableGameBoy::getOAMDebug() {
    return oam;
}

IMemoryDebug &DebuggableGameBoy::getHRAMDebug() {
    return hram;
}

IBootIODebug &DebuggableGameBoy::getBootIODebug() {
    return boot;
}

IInterruptsIODebug &DebuggableGameBoy::getInterruptsIODebug() {
    return interrupts;
}

IJoypadIODebug &DebuggableGameBoy::getJoypadIODebug() {
    return joypad;
}

ILCDIODebug &DebuggableGameBoy::getLCDIODebug() {
    return lcd;
}

ISerialIODebug &DebuggableGameBoy::getSerialIODebug() {
    return serialPort;
}

ISoundIODebug &DebuggableGameBoy::getSoundIODebug() {
    return sound;
}

ITimersIODebug &DebuggableGameBoy::getTimersIODebug() {
    return timers;
}


ICPUDebug &DebuggableGameBoy::getCPUDebug() {
    return cpu;
}

IPPUDebug &DebuggableGameBoy::getPPUDebug() {
    return ppu;
}

ILCDDebug &DebuggableGameBoy::getLCDDebug() {
    return lcd;
}

IMemoryDebug &DebuggableGameBoy::getCartridgeSlotDebug() {
    return cartridgeSlot;
}

IReadableDebug &DebuggableGameBoy::getBootROMDebug() {
    return boot;
}
