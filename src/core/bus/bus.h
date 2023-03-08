#ifndef BUS_H
#define BUS_H

#include <cstdint>

class IMemory;

class IBus {
public:
    virtual ~IBus() = default;
    [[nodiscard]] virtual uint8_t read(uint16_t addr) const = 0;
    virtual void write(uint16_t addr, uint8_t value) = 0;

    virtual void attachCartridge(IMemory *cartridge) = 0;
    virtual void detachCartridge() = 0;
};

class Bus : public virtual IBus {
public:
    Bus(IMemory &vram, IMemory &wram1, IMemory &wram2, IMemory &oam, IMemory &io, IMemory &hram, IMemory &ie);
    ~Bus() override = default;

    [[nodiscard]] uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;

    void attachCartridge(IMemory *cartridge) override;
    void detachCartridge() override;

private:
    IMemory *cartridge;
    IMemory &vram;
    IMemory &wram1;
    IMemory &wram2;
    IMemory &oam;
    IMemory &io;
    IMemory &hram;
    IMemory &ie;
};

#endif // BUS_H