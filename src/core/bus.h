#ifndef BUS_H
#define BUS_H


class Cartridge;
class Memory;

#include <cstdint>

class Bus {
public:
    Bus(Cartridge &cartridge, Memory &memory);

    [[nodiscard]] uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t value);

private:
    Cartridge &cartridge;
    Memory &memory;
};

#endif // BUS_H