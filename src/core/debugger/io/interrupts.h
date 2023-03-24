#ifndef DEBUGGERINTERRUPTSIO_H
#define DEBUGGERINTERRUPTSIO_H

#include <cstdint>
#include "utils/binutils.h"

class IInterruptsIODebug {
public:
    class Observer {
    public:
        virtual ~Observer() = default;

        virtual void onReadIF(uint8_t value) = 0;
        virtual void onWriteIF(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadIE(uint8_t value) = 0;
        virtual void onWriteIE(uint8_t oldValue, uint8_t newValue) = 0;
    };

    virtual ~IInterruptsIODebug() = default;

    virtual void setObserver(Observer *observer) = 0;
};

#endif // DEBUGGERINTERRUPTSIO_H