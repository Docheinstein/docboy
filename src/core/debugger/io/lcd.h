#ifndef DEBUGGERLCDIO_H
#define DEBUGGERLCDIO_H

#include <cstdint>

class ILCDIODebug {
public:
    class Observer {
    public:
        virtual ~Observer() = default;

        virtual void onReadLCDC(uint8_t value) = 0;
        virtual void onWriteLCDC(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadSTAT(uint8_t value) = 0;
        virtual void onWriteSTAT(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadSCY(uint8_t value) = 0;
        virtual void onWriteSCY(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadSCX(uint8_t value) = 0;
        virtual void onWriteSCX(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadLY(uint8_t value) = 0;
        virtual void onWriteLY(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadLYC(uint8_t value) = 0;
        virtual void onWriteLYC(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadDMA(uint8_t value) = 0;
        virtual void onWriteDMA(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadBGP(uint8_t value) = 0;
        virtual void onWriteBGP(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadOBP0(uint8_t value) = 0;
        virtual void onWriteOBP0(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadOBP1(uint8_t value) = 0;
        virtual void onWriteOBP1(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWY(uint8_t value) = 0;
        virtual void onWriteWY(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWX(uint8_t value) = 0;
        virtual void onWriteWX(uint8_t oldValue, uint8_t newValue) = 0;
    };

    virtual ~ILCDIODebug() = default;

    virtual void setObserver(Observer *observer) = 0;
};

#endif // DEBUGGERLCDIO_H