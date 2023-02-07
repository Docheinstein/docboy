#ifndef GIBLE_H
#define GIBLE_H

#include <string>
#include "memory.h"
#include "cartridge.h"
#include "cpu.h"
#include "bus.h"

class Gible {
public:
    Gible();

    bool loadROM(const std::string &rom);
    void start();

private:
    Cartridge cartridge;
    Memory memory;
    BusImpl bus;
    CPU cpu;
};
#endif // GIBLE_H