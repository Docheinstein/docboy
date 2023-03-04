#ifndef DEBUGGABLEBUS_H
#define DEBUGGABLEBUS_H

#include "core/bus.h"

class IDebuggableBus : public virtual IBus {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void onBusRead(uint16_t addr, uint8_t value) = 0;
        virtual void onBusWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) = 0;
    };

    virtual void setObserver(Observer *observer) = 0;
    virtual void unsetObserver() = 0;

    [[nodiscard]] virtual uint8_t read(uint16_t addr, bool notify) const = 0;
    virtual void write(uint16_t addr, uint8_t value, bool notify) = 0;

    [[nodiscard]] uint8_t read(uint16_t addr) const override = 0;
    void write(uint16_t addr, uint8_t value) override = 0;
};

class DebuggableBus : public Bus, public IDebuggableBus {
public:
    DebuggableBus(IMemory &vram, IMemory &wram1, IMemory &wram2, IMemory &oam, IMemory &io, IMemory &hram, IMemory &ie);
    ~DebuggableBus() override = default;

    void setObserver(Observer *observer) override;
    void unsetObserver() override;

    [[nodiscard]] uint8_t read(uint16_t addr, bool notify) const override;
    void write(uint16_t addr, uint8_t value, bool notify) override;

    [[nodiscard]] uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;

private:
    Observer *observer;
};

#endif // DEBUGGABLEBUS_H