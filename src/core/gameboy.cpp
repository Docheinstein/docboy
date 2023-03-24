#include "gameboy.h"

GameBoy::Builder::Builder(): bootRom(), frequency() {

}

GameBoy::Builder &GameBoy::Builder::setBootROM(std::unique_ptr<IBootROM> bootRom_) {
    bootRom = std::move(bootRom_);
    return *this;
}

GameBoy::Builder &GameBoy::Builder::setFrequency(uint64_t frequency_) {
    frequency = frequency_;
    return *this;
}

GameBoy GameBoy::Builder::build() {
    return GameBoy(std::move(bootRom), frequency);
}

GameBoy::GameBoy(std::unique_ptr<IBootROM> bootRom_, uint64_t frequency) :
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

IMemory &GameBoy::getVRAM() {
    return vram;
}

IMemory &GameBoy::getWRAM1() {
    return wram1;
}

IMemory &GameBoy::getWRAM2() {
    return wram2;
}

IMemory &GameBoy::getOAM() {
    return oam;
}

IMemory &GameBoy::getHRAM() {
    return hram;
}

IBootIO &GameBoy::getBootIO() {
    return boot;
}

IInterruptsIO &GameBoy::getInterruptsIO() {
    return interrupts;
}

IJoypadIO &GameBoy::getJoypadIO() {
    return joypad;
}

ILCDIO &GameBoy::getLCDIO() {
    return lcd;
}

ISerialIO &GameBoy::getSerialIO() {
    return serialPort;
}

ISoundIO &GameBoy::getSoundIO() {
    return sound;
}

ITimersIO &GameBoy::getTimersIO() {
    return timers;
}

ICPU &GameBoy::getCPU() {
    return cpu;
}

IPPU &GameBoy::getPPU() {
    return ppu;
}

IClock &GameBoy::getClock() {
    return clock;
}

IJoypad &GameBoy::getJoypad() {
    return joypad;
}

ISerialPort &GameBoy::getSerialPort() {
    return serialPort;
}

ICartridgeSlot &GameBoy::getCartridgeSlot() {
    return cartridgeSlot;
}

IBus &GameBoy::getBus() {
    return bus;
}
