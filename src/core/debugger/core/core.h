#ifndef DEBUGGERCORE_H
#define DEBUGGERCORE_H

#include "core/core.h"
#include "core/debugger/memory/memory.h"

class IDebuggableCPU;
class IDebuggablePPU;
class IDebuggableLCD;
class IBus;

class IDebuggableCore {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual bool onTick() = 0;
        virtual void onMemoryRead(uint16_t addr, uint8_t value) = 0;
        virtual void onMemoryWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) = 0;
    };
    virtual ~IDebuggableCore() = default;

    virtual void setObserver(Observer *observer) = 0;

    [[nodiscard]] virtual IDebuggableCPU & getCpu() = 0;
    [[nodiscard]] virtual IDebuggablePPU & getPpu() = 0;
    [[nodiscard]] virtual IDebuggableLCD & getLcd() = 0;
    [[nodiscard]] virtual IBus & getBus() = 0;
    [[nodiscard]] virtual uint64_t getClock() = 0;
};

class DebuggableCore : public Core, public IDebuggableCore, public IDebuggableMemory::Observer  {
public:
    explicit DebuggableCore(GameBoy &gb);
    ~DebuggableCore() override = default;

    void setObserver(IDebuggableCore::Observer *observer) override;

    void onRead(uint16_t addr, uint8_t value) override;
    void onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) override;

    [[nodiscard]] IDebuggableCPU & getCpu() override;
    [[nodiscard]] IDebuggablePPU & getPpu() override;
    [[nodiscard]] IDebuggableLCD & getLcd() override;
    [[nodiscard]] IBus & getBus() override;
    uint64_t getClock() override;

    bool isOn() override;
    void tick() override;

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

    bool on;
};

#endif // DEBUGGERCORE_H