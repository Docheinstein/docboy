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

class BusImpl : public IBus {
public:
    BusImpl(Cartridge &cartridge, Memory &memory);
    ~BusImpl() override;

    [[nodiscard]]

    uint8_t read(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

private:
    Cartridge &cartridge;
    Memory &memory;
};

#ifndef TESTING
#define TESTING 0
#endif

//typedef std::conditional<TESTING, IBus, BusImpl>::type  Bus;

#ifdef TESTING
using Bus = IBus;
#else
using Bus = BusImpl;
#endif

#endif // BUS_H