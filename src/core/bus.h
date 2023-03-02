#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <type_traits>
#include <memory>

class IMemory;

class IBus {
public:
    virtual ~IBus() = default;
    [[nodiscard]] virtual uint8_t read(uint16_t addr) const = 0;
    virtual void write(uint16_t addr, uint8_t value) = 0;
};

class Bus : public virtual IBus {
public:
    Bus(IMemory &wram1, IMemory &wram2, IMemory &io, IMemory &hram, IMemory &ie);
    ~Bus() override = default;

    [[nodiscard]] uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;

    void attachCartridge(IMemory *cartridge);
    void detachCartridge();

private:
    IMemory &wram1;
    IMemory &wram2;
    IMemory &io;
    IMemory &hram;
    IMemory &ie;
    IMemory *cartridge;
};


#endif // BUS_H