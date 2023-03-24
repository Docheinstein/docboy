#ifndef DEBUGGERSERIALIO_H
#define DEBUGGERSERIALIO_H

#include <cstdint>

class ISerialIODebug {
public:
    class Observer {
    public:
        virtual ~Observer() = default;

        virtual void onReadSB(uint8_t value) = 0;
        virtual void onWriteSB(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadSC(uint8_t value) = 0;
        virtual void onWriteSC(uint8_t oldValue, uint8_t newValue) = 0;
    };

    virtual ~ISerialIODebug() = default;

    virtual void setObserver(Observer *observer) = 0;
};

#endif // DEBUGGERSERIALIO_H