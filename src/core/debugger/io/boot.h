#ifndef DEBUGGERBOOTIO_H
#define DEBUGGERBOOTIO_H

#include <cstdint>

class IBootIODebug {
public:
    class Observer {
    public:
        virtual ~Observer() = default;

        virtual void onReadBOOT(uint8_t value) = 0;
        virtual void onWriteBOOT(uint8_t oldValue, uint8_t newValue) = 0;
    };

    virtual ~IBootIODebug() = default;

    virtual void setObserver(Observer *observer) = 0;
};

#endif // DEBUGGERBOOTIO_H