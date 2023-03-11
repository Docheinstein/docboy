#ifndef GAMEBOY_H
#define GAMEBOY_H

#include <memory>
#include "definitions.h"
#include "impl/bootrom.h"
#include "impl/cpu.h"
#include "impl/lcd.h"
#include "impl/memory.h"
#include "impl/ppu.h"
#include "bus/bus.h"
#include "clock/clock.h"
#include "memory/io.h"
#include "cartridge/cartridge.h"
#include "serial/port.h"
#include "serial/link.h"

class GameBoy {
public:
    class Builder {
    public:
        Builder & setFrequency(uint64_t frequency);
        Builder & setBootROM(std::unique_ptr<Impl::IBootROM> bootRom);
        Builder & setLCD(std::shared_ptr<Impl::ILCD> lcd);
        GameBoy build();

    private:
        uint64_t frequency;
        std::unique_ptr<Impl::IBootROM> bootRom;
        std::shared_ptr<Impl::ILCD> lcd;
    };

    Impl::Memory vram;
    Impl::Memory wram1;
    Impl::Memory wram2;
    Impl::Memory oam;
    IO io;
    Impl::Memory hram;
    Impl::Memory ie;

    Bus bus;

    SerialPort serialPort;

    Impl::CPU cpu;

    std::shared_ptr<Impl::ILCD> lcd;
    Impl::PPU ppu;

    Clock clock;

    std::unique_ptr<Cartridge> cartridge;

    void attachCartridge(std::unique_ptr<Cartridge> cartridge);
    void detachCartridge();

    void attachSerialLink(SerialLink::Plug &plug);
    void detachSerialLink();

private:
    explicit GameBoy(
            std::shared_ptr<Impl::ILCD> lcd = nullptr,
            std::unique_ptr<Impl::IBootROM> bootRom = nullptr,
            uint64_t cpuFrequency = Specs::CPU::FREQUENCY,
            uint64_t ppuFrequency = Specs::PPU::FREQUENCY);

};

#endif // GAMEBOY_H