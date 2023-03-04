#ifndef DEBUGGABLECORE_H
#define DEBUGGABLECORE_H

#include "core/core.h"

class IDebuggableBus;
class IDebuggableCPU;

class IDebuggableCore {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual bool onTick() = 0;
    };
    virtual ~IDebuggableCore() = default;

    virtual void setObserver(Observer *observer) = 0;
    virtual void unsetObserver() = 0;

//    [[nodiscard]] virtual IDebuggableBus &getBus() = 0;
    [[nodiscard]] virtual IDebuggableCPU &getCpu() = 0;
};

class DebuggableCore : public Core, public IDebuggableCore {
public:
    explicit DebuggableCore(GameBoy &gb);
    ~DebuggableCore() override = default;

    void setObserver(Observer *observer) override;
    void unsetObserver() override;

//    [[nodiscard]] IDebuggableBus & getBus() override;
    [[nodiscard]] IDebuggableCPU & getCpu() override;

    void tick() override;

private:
    Observer *observer;
};

#endif // DEBUGGABLECORE_H