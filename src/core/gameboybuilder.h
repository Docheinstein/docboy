#ifndef GAMEBOYBUILDER_H
#define GAMEBOYBUILDER_H

#include "gameboy.h"
#include "boot/bootrom.h"
#include "cpu/cpu.h"

class GameBoyBuilder {
public:
    GameBoyBuilder() = default;

    GameBoyBuilder & setBootROM(std::unique_ptr<IBootROM> bootRom);
    GameBoyBuilder & setLCD(std::unique_ptr<ILCDImpl> lcd);
    GameBoy build();

private:
    std::unique_ptr<IBootROM> bootRom;
    std::unique_ptr<IBus> bus;
    std::unique_ptr<ICPUImpl> cpu;
    std::unique_ptr<ILCDImpl> lcd;
    std::unique_ptr<ICPUImpl> ppu;
};

#endif // GAMEBOYBUILDER_H