#ifndef DEBUGGERCORE_H
#define DEBUGGERCORE_H

#include "core/core.h"
#include "core/debugger/io/boot.h"
#include "core/debugger/io/interrupts.h"
#include "core/debugger/io/joypad.h"
#include "core/debugger/io/lcd.h"
#include "core/debugger/io/serial.h"
#include "core/debugger/io/sound.h"
#include "core/debugger/io/timers.h"
#include "core/debugger/memory/memory.h"

class IDebuggableGameBoy;

class ICoreDebug {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual bool onTick(uint64_t tick) = 0;
        virtual void onMemoryRead(uint16_t addr, uint8_t value) = 0;
        virtual void onMemoryWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) = 0;
    };
    virtual ~ICoreDebug() = default;

    virtual IDebuggableGameBoy& getGameBoy() = 0;
    virtual void setObserver(Observer* observer) = 0;
    virtual Observer* getObserver() = 0;
};

class DebuggableCore : public ICoreDebug, public Core, private IMemoryDebug::Observer {
public:
    explicit DebuggableCore(IDebuggableGameBoy& gameboy);
    ~DebuggableCore() override = default;

    IDebuggableGameBoy& getGameBoy() override;
    void setObserver(ICoreDebug::Observer* observer) override;
    ICoreDebug::Observer* getObserver() override;

    void tick() override;
    bool isOn() override;

private:
    class MemoryObserver : public IMemoryDebug::Observer {
    public:
        explicit MemoryObserver(IMemoryDebug::Observer& observer, uint16_t base = 0);
        void onRead(uint16_t addr, uint8_t value) override;
        void onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) override;

    private:
        IMemoryDebug::Observer& observer;
        uint16_t base;
    };

    class IOObserver : public IInterruptsIODebug::Observer,
                       public IJoypadIODebug::Observer,
                       public ILCDIODebug::Observer,
                       public ISerialIODebug::Observer,
                       public ISoundIODebug::Observer,
                       public ITimersIODebug::Observer,
                       public IBootIODebug::Observer {
    public:
        explicit IOObserver(IMemoryDebug::Observer& delegate);

        // interrupts
        void onReadIF(uint8_t value) override;
        void onWriteIF(uint8_t oldValue, uint8_t newValue) override;

        void onReadIE(uint8_t value) override;
        void onWriteIE(uint8_t oldValue, uint8_t newValue) override;

        // joypad
        void onReadP1(uint8_t value) override;
        void onWriteP1(uint8_t oldValue, uint8_t newValue) override;

        // lcd
        void onReadLCDC(uint8_t value) override;
        void onWriteLCDC(uint8_t oldValue, uint8_t newValue) override;

        void onReadSTAT(uint8_t value) override;
        void onWriteSTAT(uint8_t oldValue, uint8_t newValue) override;

        void onReadSCY(uint8_t value) override;
        void onWriteSCY(uint8_t oldValue, uint8_t newValue) override;

        void onReadSCX(uint8_t value) override;
        void onWriteSCX(uint8_t oldValue, uint8_t newValue) override;

        void onReadLY(uint8_t value) override;
        void onWriteLY(uint8_t oldValue, uint8_t newValue) override;

        void onReadLYC(uint8_t value) override;
        void onWriteLYC(uint8_t oldValue, uint8_t newValue) override;

        void onReadDMA(uint8_t value) override;
        void onWriteDMA(uint8_t oldValue, uint8_t newValue) override;

        void onReadBGP(uint8_t value) override;
        void onWriteBGP(uint8_t oldValue, uint8_t newValue) override;

        void onReadOBP0(uint8_t value) override;
        void onWriteOBP0(uint8_t oldValue, uint8_t newValue) override;

        void onReadOBP1(uint8_t value) override;
        void onWriteOBP1(uint8_t oldValue, uint8_t newValue) override;

        void onReadWY(uint8_t value) override;
        void onWriteWY(uint8_t oldValue, uint8_t newValue) override;

        void onReadWX(uint8_t value) override;
        void onWriteWX(uint8_t oldValue, uint8_t newValue) override;

        // serial
        void onReadSB(uint8_t value) override;
        void onWriteSB(uint8_t oldValue, uint8_t newValue) override;

        void onReadSC(uint8_t value) override;
        void onWriteSC(uint8_t oldValue, uint8_t newValue) override;

        // sound
        void onReadNR10(uint8_t value) override;
        void onWriteNR10(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR11(uint8_t value) override;
        void onWriteNR11(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR12(uint8_t value) override;
        void onWriteNR12(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR13(uint8_t value) override;
        void onWriteNR13(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR14(uint8_t value) override;
        void onWriteNR14(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR21(uint8_t value) override;
        void onWriteNR21(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR22(uint8_t value) override;
        void onWriteNR22(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR23(uint8_t value) override;
        void onWriteNR23(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR24(uint8_t value) override;
        void onWriteNR24(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR30(uint8_t value) override;
        void onWriteNR30(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR31(uint8_t value) override;
        void onWriteNR31(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR32(uint8_t value) override;
        void onWriteNR32(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR33(uint8_t value) override;
        void onWriteNR33(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR34(uint8_t value) override;
        void onWriteNR34(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR41(uint8_t value) override;
        void onWriteNR41(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR42(uint8_t value) override;
        void onWriteNR42(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR43(uint8_t value) override;
        void onWriteNR43(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR44(uint8_t value) override;
        void onWriteNR44(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR50(uint8_t value) override;
        void onWriteNR50(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR51(uint8_t value) override;
        void onWriteNR51(uint8_t oldValue, uint8_t newValue) override;

        void onReadNR52(uint8_t value) override;
        void onWriteNR52(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVE0(uint8_t value) override;
        void onWriteWAVE0(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVE1(uint8_t value) override;
        void onWriteWAVE1(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVE2(uint8_t value) override;
        void onWriteWAVE2(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVE3(uint8_t value) override;
        void onWriteWAVE3(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVE4(uint8_t value) override;
        void onWriteWAVE4(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVE5(uint8_t value) override;
        void onWriteWAVE5(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVE6(uint8_t value) override;
        void onWriteWAVE6(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVE7(uint8_t value) override;
        void onWriteWAVE7(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVE8(uint8_t value) override;
        void onWriteWAVE8(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVE9(uint8_t value) override;
        void onWriteWAVE9(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVEA(uint8_t value) override;
        void onWriteWAVEA(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVEB(uint8_t value) override;
        void onWriteWAVEB(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVEC(uint8_t value) override;
        void onWriteWAVEC(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVED(uint8_t value) override;
        void onWriteWAVED(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVEE(uint8_t value) override;
        void onWriteWAVEE(uint8_t oldValue, uint8_t newValue) override;

        void onReadWAVEF(uint8_t value) override;
        void onWriteWAVEF(uint8_t oldValue, uint8_t newValue) override;

        // timers
        void onReadDIV(uint8_t value) override;
        void onWriteDIV(uint8_t oldValue, uint8_t newValue) override;

        void onReadTIMA(uint8_t value) override;
        void onWriteTIMA(uint8_t oldValue, uint8_t newValue) override;

        void onReadTMA(uint8_t value) override;
        void onWriteTMA(uint8_t oldValue, uint8_t newValue) override;

        void onReadTAC(uint8_t value) override;
        void onWriteTAC(uint8_t oldValue, uint8_t newValue) override;

        // boot
        void onReadBOOT(uint8_t value) override;
        void onWriteBOOT(uint8_t oldValue, uint8_t newValue) override;

    private:
        IMemoryDebug::Observer& observer;
    };

    void onWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) override;
    void onRead(uint16_t addr, uint8_t value) override;

    ICoreDebug::Observer* observer;

    MemoryObserver bootRomObserver;
    MemoryObserver cartridgeObserver;

    MemoryObserver vramObserver;
    MemoryObserver wram1Observer;
    MemoryObserver wram2Observer;
    MemoryObserver oamObserver;
    MemoryObserver hramObserver;

    IOObserver ioObserver;

    bool on;
};

#endif // DEBUGGERCORE_H