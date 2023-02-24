#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <type_traits>
#include <memory>

class Cartridge;
class IMemory;
class ISerial;


class IBus {
public:
    virtual ~IBus() = default;
    virtual uint8_t read(uint16_t addr) = 0;
    virtual void write(uint16_t addr, uint8_t value) = 0;
};

class Bus : public IBus {
public:
    Bus(IMemory &wram1, IMemory &wram2, IMemory &io, IMemory &hram, IMemory &ie);
    ~Bus() override = default;

    uint8_t read(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

    void attachCartridge(const std::shared_ptr<Cartridge> &cartridge);
    void detachCartridge();

private:
    std::shared_ptr<Cartridge> cartridge;
    IMemory &wram1;
    IMemory &wram2;
    IMemory &io;
    IMemory &hram;
    IMemory &ie;
};

class ObservableBus : public Bus {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void onBusRead(uint16_t addr, uint8_t value) = 0;
        virtual void onBusWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) = 0;
    };
    ObservableBus(IMemory &wram1, IMemory &wram2, IMemory &io, IMemory &hram, IMemory &ie);
    ~ObservableBus() override = default;

    void setObserver(Observer *observer);
    void unsetObserver();

    uint8_t read(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

private:
    Observer *observer;
};

#endif // BUS_H