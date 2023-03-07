#ifndef DEBUGGABLECORE_H
#define DEBUGGABLECORE_H

#include "core/core.h"
#include "debuggablememory.h"
#include "debuggablelcd.h"

class IDebuggableCPU;
class IDebuggablePPU;

class IDebuggableCore {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual bool onTick(uint8_t clk) = 0;
        virtual void onMemoryRead(uint16_t addr, uint8_t value) = 0;
        virtual void onMemoryWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) = 0;
    };
    virtual ~IDebuggableCore() = default;

    virtual void setObserver(Observer *observer) = 0;

    [[nodiscard]] virtual IDebuggableCPU & getCpu() = 0;
    [[nodiscard]] virtual IDebuggablePPU & getPpu() = 0;
    [[nodiscard]] virtual IBus & getBus() = 0;
    [[nodiscard]] virtual IDebuggableLCD & getLcd() = 0;
};

class DebuggableCore : public Core, public IDebuggableCore, public DebuggableMemory::Observer  {
public:
    explicit DebuggableCore(GameBoy &gb);
    ~DebuggableCore() override = default;

    void setObserver(IDebuggableCore::Observer *observer) override;

    void onRead(uint16_t addr, uint8_t value) override;
    void onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) override;

    [[nodiscard]] IDebuggableCPU & getCpu() override;
    [[nodiscard]] IDebuggablePPU & getPpu() override;
    [[nodiscard]] IBus & getBus() override;
    [[nodiscard]] IDebuggableLCD & getLcd() override;

    bool tick() override;

protected:
    void attachCartridge(std::unique_ptr<Cartridge> cartridge) override;

private:
    IDebuggableCore::Observer *observer;

    class MemoryObserver : public DebuggableMemory::Observer {
    public:
        MemoryObserver(DebuggableMemory::Observer *observer, uint16_t base);
        void onRead(uint16_t addr, uint8_t value) override;
        void onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) override;
    private:
        DebuggableMemory::Observer *observer;
        uint16_t base;
    };

    MemoryObserver bootromObserver;
    MemoryObserver cartridgeObserver;
    MemoryObserver vramObserver;
    MemoryObserver wram1Observer;
    MemoryObserver wram2Observer;
    MemoryObserver oamObserver;
    MemoryObserver ioObserver;
    MemoryObserver hramObserver;
    MemoryObserver ieObserver;
};

#endif // DEBUGGABLECORE_H