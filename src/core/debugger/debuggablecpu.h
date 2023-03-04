#ifndef DEBUGGABLECpu_H
#define DEBUGGABLECpu_H

#include "core/cpu/cpu.h"

class IDebuggableCPU : public virtual ICPU {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void onMemoryRead(uint16_t addr, uint8_t value) = 0;
        virtual void onMemoryWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) = 0;
    };

    struct Registers {
        uint16_t AF;
        uint16_t BC;
        uint16_t DE;
        uint16_t HL;
        uint16_t PC;
        uint16_t SP;
    };

    struct Instruction {
        bool ISR;
        uint16_t address;
        uint8_t microop;
    };

    ~IDebuggableCPU() override = default;

    virtual void setObserver(Observer *observer) = 0;
    virtual void unsetObserver() = 0;

    [[nodiscard]] virtual Registers getRegisters() const = 0;
    [[nodiscard]] virtual Instruction getCurrentInstruction() const = 0;
    [[nodiscard]] virtual bool getIME() const = 0;
    [[nodiscard]] virtual bool isHalted() const = 0;
    [[nodiscard]] virtual uint64_t getCycles() const = 0;

     // TODO: better name? IDebuggableCPU::readMemory clashes with CPU::readMemory, but they do different things
    [[nodiscard]] virtual uint8_t readMemoryRaw(uint16_t addr) const = 0;
};

class DebuggableCPU : public IDebuggableCPU, public CPU {
public:
    explicit DebuggableCPU(IBus &bus, std::unique_ptr<IBootROM> bootRom = nullptr); // TODO: maybe take IO, ... separately?
    ~DebuggableCPU() override = default;

    void setObserver(Observer *observer) override;
    void unsetObserver() override;

    [[nodiscard]] Registers getRegisters() const override;
    [[nodiscard]] Instruction getCurrentInstruction() const override;
    [[nodiscard]] bool getIME() const override;
    [[nodiscard]] bool isHalted() const override;
    [[nodiscard]] uint64_t getCycles() const override;

    [[nodiscard]] uint8_t readMemoryRaw(uint16_t addr) const override;

    uint8_t readMemory(uint16_t addr, bool notify) const;
    void writeMemory(uint16_t addr, uint8_t value, bool notify);

    uint8_t readMemory(uint16_t addr) const override;
    void writeMemory(uint16_t addr, uint8_t value) override;

private:
    Observer *observer;
};

#endif // DEBUGGABLECpu_H