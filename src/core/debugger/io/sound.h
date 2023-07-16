#ifndef DEBUGGERSOUNDIO_H
#define DEBUGGERSOUNDIO_H

#include <cstdint>

class ISoundIODebug {
public:
    class Observer {
    public:
        virtual ~Observer() = default;

        virtual void onReadNR10(uint8_t value) = 0;
        virtual void onWriteNR10(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR11(uint8_t value) = 0;
        virtual void onWriteNR11(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR12(uint8_t value) = 0;
        virtual void onWriteNR12(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR13(uint8_t value) = 0;
        virtual void onWriteNR13(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR14(uint8_t value) = 0;
        virtual void onWriteNR14(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR21(uint8_t value) = 0;
        virtual void onWriteNR21(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR22(uint8_t value) = 0;
        virtual void onWriteNR22(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR23(uint8_t value) = 0;
        virtual void onWriteNR23(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR24(uint8_t value) = 0;
        virtual void onWriteNR24(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR30(uint8_t value) = 0;
        virtual void onWriteNR30(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR31(uint8_t value) = 0;
        virtual void onWriteNR31(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR32(uint8_t value) = 0;
        virtual void onWriteNR32(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR33(uint8_t value) = 0;
        virtual void onWriteNR33(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR34(uint8_t value) = 0;
        virtual void onWriteNR34(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR41(uint8_t value) = 0;
        virtual void onWriteNR41(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR42(uint8_t value) = 0;
        virtual void onWriteNR42(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR43(uint8_t value) = 0;
        virtual void onWriteNR43(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR44(uint8_t value) = 0;
        virtual void onWriteNR44(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR50(uint8_t value) = 0;
        virtual void onWriteNR50(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR51(uint8_t value) = 0;
        virtual void onWriteNR51(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadNR52(uint8_t value) = 0;
        virtual void onWriteNR52(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVE0(uint8_t value) = 0;
        virtual void onWriteWAVE0(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVE1(uint8_t value) = 0;
        virtual void onWriteWAVE1(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVE2(uint8_t value) = 0;
        virtual void onWriteWAVE2(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVE3(uint8_t value) = 0;
        virtual void onWriteWAVE3(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVE4(uint8_t value) = 0;
        virtual void onWriteWAVE4(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVE5(uint8_t value) = 0;
        virtual void onWriteWAVE5(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVE6(uint8_t value) = 0;
        virtual void onWriteWAVE6(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVE7(uint8_t value) = 0;
        virtual void onWriteWAVE7(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVE8(uint8_t value) = 0;
        virtual void onWriteWAVE8(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVE9(uint8_t value) = 0;
        virtual void onWriteWAVE9(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVEA(uint8_t value) = 0;
        virtual void onWriteWAVEA(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVEB(uint8_t value) = 0;
        virtual void onWriteWAVEB(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVEC(uint8_t value) = 0;
        virtual void onWriteWAVEC(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVED(uint8_t value) = 0;
        virtual void onWriteWAVED(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVEE(uint8_t value) = 0;
        virtual void onWriteWAVEE(uint8_t oldValue, uint8_t newValue) = 0;

        virtual void onReadWAVEF(uint8_t value) = 0;
        virtual void onWriteWAVEF(uint8_t oldValue, uint8_t newValue) = 0;
    };

    virtual ~ISoundIODebug() = default;

    virtual void setObserver(Observer* observer) = 0;
};

#endif // DEBUGGERSOUNDIO_H