#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <type_traits>
class Cartridge;
class IMemory;
class ISerial;


class IBus {
public:
    virtual ~IBus();
    virtual uint8_t read(uint16_t addr) = 0;
    virtual void write(uint16_t addr, uint8_t value) = 0;
};

class Bus : public IBus {
public:
    Bus(Cartridge &cartridge, IMemory &wram1, IMemory &wram2, IMemory &io, IMemory &hram, IMemory &ie);
    ~Bus() override;

    uint8_t read(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

private:
    Cartridge &cartridge;
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
        ~Observer();
        virtual void onBusRead(uint16_t addr, uint8_t value) = 0;
        virtual void onBusWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) = 0;
    };
    ObservableBus(Cartridge &cartridge, IMemory &wram1, IMemory &wram2, IMemory &io, IMemory &hram, IMemory &ie);
    ~ObservableBus() override;

    void setObserver(Observer *observer);
    void unsetObserver();

    uint8_t read(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

private:
    Observer *observer;
};

#endif // BUS_H