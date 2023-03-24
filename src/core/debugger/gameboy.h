#ifndef DEBUGGERGAMEBOY_H
#define DEBUGGERGAMEBOY_H

#include "core/gameboy.h"
#include "cpu/cpu.h"
#include "cpu/interrupts.h"
#include "core/debugger/memory/memory.h"
#include "io/boot.h"
#include "io/interrupts.h"
#include "io/joypad.h"
#include "io/lcd.h"
#include "io/serial.h"
#include "io/sound.h"
#include "io/timers.h"
#include "boot/boot.h"
#include "core/debugger/joypad/joypad.h"
#include "core/debugger/lcd/framebufferlcd.h"
#include "core/debugger/cpu/timers.h"
#include "core/debugger/sound/sound.h"
#include "core/debugger/serial/port.h"
#include "core/debugger/ppu/ppu.h"
#include "core/debugger/cartridge/slot.h"

class IDebuggableGameBoy : public IGameBoy {
public:
    virtual IMemoryDebug &getVRAMDebug() = 0;
    virtual IMemoryDebug &getWRAM1Debug() = 0;
    virtual IMemoryDebug &getWRAM2Debug() = 0;
    virtual IMemoryDebug &getOAMDebug() = 0;
    virtual IMemoryDebug &getHRAMDebug() = 0;

    virtual IBootIODebug &getBootIODebug() = 0;
    virtual IInterruptsIODebug &getInterruptsIODebug() = 0;
    virtual IJoypadIODebug &getJoypadIODebug() = 0;
    virtual ILCDIODebug &getLCDIODebug() = 0;
    virtual ISerialIODebug &getSerialIODebug() = 0;
    virtual ISoundIODebug &getSoundIODebug() = 0;
    virtual ITimersIODebug &getTimersIODebug() = 0;

    virtual ICPUDebug &getCPUDebug() = 0;
    virtual IPPUDebug &getPPUDebug() = 0;
    virtual ILCDDebug &getLCDDebug() = 0;

    virtual IMemoryDebug &getCartridgeSlotDebug() = 0;
    virtual IReadableDebug &getBootROMDebug() = 0;
};

class DebuggableGameBoy : public IDebuggableGameBoy {
public:
    class Builder {
    public:
        Builder();
        Builder & setBootROM(std::unique_ptr<IBootROM> bootRom);
        Builder & setFrequency(uint64_t frequency);
        DebuggableGameBoy build();
    private:
        std::unique_ptr<IBootROM> bootRom;
        uint64_t frequency;
    };

    explicit DebuggableGameBoy(
            std::unique_ptr<IBootROM> bootRom_ = nullptr,
            uint64_t frequency = Specs::FREQUENCY);

    IMemory &getVRAM() override;
    IMemory &getWRAM1() override;
    IMemory &getWRAM2() override;
    IMemory &getOAM() override;
    IMemory &getHRAM() override;

    IBootIO &getBootIO() override;
    IInterruptsIO &getInterruptsIO() override;
    IJoypadIO &getJoypadIO() override;
    ILCDIO &getLCDIO() override;
    ISerialIO &getSerialIO() override;
    ISoundIO &getSoundIO() override;
    ITimersIO &getTimersIO() override;

    ICPU &getCPU() override;
    IPPU &getPPU() override;
    IClock &getClock() override;

    IJoypad &getJoypad() override;
    ISerialPort &getSerialPort() override;
    ICartridgeSlot &getCartridgeSlot() override;

    IBus &getBus() override;

    IMemoryDebug &getVRAMDebug() override;
    IMemoryDebug &getWRAM1Debug() override;
    IMemoryDebug &getWRAM2Debug() override;
    IMemoryDebug &getOAMDebug() override;
    IMemoryDebug &getHRAMDebug() override;

    IBootIODebug &getBootIODebug() override;
    IInterruptsIODebug &getInterruptsIODebug() override;
    IJoypadIODebug &getJoypadIODebug() override;
    ILCDIODebug &getLCDIODebug() override;
    ISerialIODebug &getSerialIODebug() override;
    ISoundIODebug &getSoundIODebug() override;
    ITimersIODebug &getTimersIODebug() override;

    ICPUDebug &getCPUDebug() override;
    IPPUDebug &getPPUDebug() override;
    ILCDDebug &getLCDDebug() override;

    IMemoryDebug &getCartridgeSlotDebug() override;
    IReadableDebug &getBootROMDebug() override;


    DebuggableMemory vram;
    DebuggableMemory wram1;
    DebuggableMemory wram2;
    DebuggableMemory oam;
    DebuggableMemory hram;

    DebuggableInterrupts interrupts;
    DebuggableJoypad joypad;
    DebuggableFrameBufferLCD lcd;
    DebuggableSerialPort serialPort;
    DebuggableTimers timers;
    DebuggableSound sound;
    DebuggableBoot boot;
    DebuggableCartridgeSlot cartridgeSlot;
    Bus bus;
    DebuggableCPU cpu;
    DebuggablePPU ppu;
    Clock clock;
};

#endif // DEBUGGERGAMEBOY_H