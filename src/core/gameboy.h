#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "core/boot/boot.h"
#include "core/bus/bus.h"
#include "core/cartridge/slot.h"
#include "core/clock/clock.h"
#include "core/cpu/cpu.h"
#include "core/cpu/interrupts.h"
#include "core/cpu/timers.h"
#include "core/dma/dma.h"
#include "core/joypad/joypad.h"
#include "core/lcd/framebufferlcd.h"
#include "core/memory/memory.h"
#include "core/ppu/lcdcontroller.h"
#include "core/ppu/ppu.h"
#include "core/serial/port.h"
#include "core/sound/sound.h"

class IGameBoy {
public:
    virtual IMemory& getVRAM() = 0;
    virtual IMemory& getWRAM1() = 0;
    virtual IMemory& getWRAM2() = 0;
    virtual IMemory& getOAM() = 0;
    virtual IMemory& getHRAM() = 0;

    virtual IBootIO& getBootIO() = 0;
    virtual IInterruptsIO& getInterruptsIO() = 0;
    virtual IJoypadIO& getJoypadIO() = 0;
    virtual ILCDIO& getLCDIO() = 0;
    virtual ISerialIO& getSerialIO() = 0;
    virtual ISoundIO& getSoundIO() = 0;
    virtual ITimersIO& getTimersIO() = 0;

    virtual ICPU& getCPU() = 0;
    virtual IPPU& getPPU() = 0;
    virtual IClock& getClock() = 0;

    virtual IJoypad& getJoypad() = 0;
    virtual ISerialPort& getSerialPort() = 0;
    virtual ICartridgeSlot& getCartridgeSlot() = 0;

    virtual IBus& getBus() = 0;
};

class GameBoy : public IGameBoy {
public:
    class Builder {
    public:
        Builder();
        Builder& setBootROM(std::unique_ptr<IBootROM> bootRom);
        Builder& setFrequency(uint64_t frequency);
        GameBoy build();

    private:
        std::unique_ptr<IBootROM> bootRom;
        uint64_t frequency;
    };

    explicit GameBoy(std::unique_ptr<IBootROM> bootRom = nullptr, uint64_t frequency = Specs::FREQUENCY);

    IMemory& getVRAM() override;
    IMemory& getWRAM1() override;
    IMemory& getWRAM2() override;
    IMemory& getOAM() override;
    IMemory& getHRAM() override;

    IBootIO& getBootIO() override;
    IInterruptsIO& getInterruptsIO() override;
    IJoypadIO& getJoypadIO() override;
    ILCDIO& getLCDIO() override;
    ISerialIO& getSerialIO() override;
    ISoundIO& getSoundIO() override;
    ITimersIO& getTimersIO() override;

    ICPU& getCPU() override;
    IPPU& getPPU() override;
    IClock& getClock() override;

    IJoypad& getJoypad() override;
    ISerialPort& getSerialPort() override;
    ICartridgeSlot& getCartridgeSlot() override;

    IBus& getBus() override;

    Memory vram;
    Memory wram1;
    Memory wram2;
    Memory oam;
    Memory hram;

    Interrupts interrupts;
    Joypad joypad;
    FrameBufferLCD lcd;
    SerialPort serialPort;
    Timers timers;
    Sound sound;
    Boot boot;
    CartridgeSlot cartridgeSlot;
    Bus bus;
    CPU cpu;
    DMA dma;
    LCDController lcdController;
    PPU ppu;
    Clock clock;
};

#endif // GAMEBOY_H