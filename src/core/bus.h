#ifndef BUS_H
#define BUS_H


class Cartridge;
class Memory;

#include <cstdint>
#include <type_traits>

class IBus {
public:
    virtual ~IBus();
    virtual uint8_t read(uint16_t addr) = 0;
    virtual void write(uint16_t addr, uint8_t value) = 0;
};

class Bus : public IBus {
public:
    Bus(Cartridge &cartridge, Memory &memory);
    ~Bus() override;

    [[nodiscard]]

    uint8_t read(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

private:
    Cartridge &cartridge;
    Memory &memory;
};


#endif // BUS_H