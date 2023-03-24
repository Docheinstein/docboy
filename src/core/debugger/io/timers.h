#ifndef DEBUGGERTIMERSIO_H
#define DEBUGGERTIMERSIO_H

#include <cstdint>


class ITimersIODebug {
public:
    class Observer {
    public:
        virtual ~Observer() = default;

        virtual void onReadDIV(uint8_t value) = 0;
        virtual void onWriteDIV(uint8_t oldValue, uint8_t newValue) = 0; 

        virtual void onReadTIMA(uint8_t value) = 0;
        virtual void onWriteTIMA(uint8_t oldValue, uint8_t newValue) = 0; 

        virtual void onReadTMA(uint8_t value) = 0;
        virtual void onWriteTMA(uint8_t oldValue, uint8_t newValue) = 0; 

        virtual void onReadTAC(uint8_t value) = 0;
        virtual void onWriteTAC(uint8_t oldValue, uint8_t newValue) = 0; 
    };

    virtual ~ITimersIODebug() = default;

    virtual void setObserver(Observer *observer) = 0;
};


#endif // DEBUGGERTIMERSIO_H