#ifndef DEBUGGABLECORE_H
#define DEBUGGABLECORE_H

#include "core/core.h"

class IDebuggableBus;
class IDebuggableCpu;

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

    [[nodiscard]] virtual IDebuggableBus &getBus() = 0;
    [[nodiscard]] virtual IDebuggableCpu &getCpu() = 0;
};

class DebuggableCore : public Core, public IDebuggableCore {
public:
    DebuggableCore();
    ~DebuggableCore() override = default;

    void setObserver(Observer *observer) override;
    void unsetObserver() override;

    [[nodiscard]] IDebuggableBus & getBus() override;
    [[nodiscard]] IDebuggableCpu & getCpu() override;

    void tick() override;

private:
    Observer *observer;
};

#endif // DEBUGGABLECORE_H