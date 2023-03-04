#ifndef GAMEBOYBUILDER_H
#define GAMEBOYBUILDER_H

#include "gameboy.h"
#include "boot/bootrom.h"
#include "cpu/cpu.h"

class GameBoyBuilder {
public:
    GameBoyBuilder() = default;

    GameBoyBuilder & setBootROM(std::unique_ptr<IBootROM> bootRom);
    GameBoyBuilder & setDisplay(std::unique_ptr<IDisplay> display);
    GameBoy build();

private:
    std::unique_ptr<IBootROM> bootRom;
    std::unique_ptr<IBus> bus;
    std::unique_ptr<ICPU> cpu;
    std::unique_ptr<IDisplay> display;
    std::unique_ptr<IGPU> gpu;
};

#endif // GAMEBOYBUILDER_H