#ifndef DEBUGGERCPU_H
#define DEBUGGERCPU_H

#include "core/cpu/cpu.h"
#include "core/debugger/boot/bootrom.h"

class IDebuggableCPU : public virtual ICPU {
public:
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

    [[nodiscard]] virtual Registers getRegisters() const = 0;
    [[nodiscard]] virtual Instruction getCurrentInstruction() const = 0;
    [[nodiscard]] virtual bool getIME() const = 0;
    [[nodiscard]] virtual bool isHalted() const = 0;
    [[nodiscard]] virtual uint64_t getCycles() const = 0;
    [[nodiscard]] virtual IDebuggableBootROM *getBootROM() const = 0;

    [[nodiscard]] virtual uint8_t readMemoryThroughCPU(uint16_t addr) const = 0;
};

class DebuggableCPU : public IDebuggableCPU, public CPU {
public:
    explicit DebuggableCPU(IBus &bus, SerialPort &serial, std::unique_ptr<Impl::IBootROM> bootRom = nullptr); // TODO: maybe take IO, ... separately?
    ~DebuggableCPU() override = default;

    [[nodiscard]] Registers getRegisters() const override;
    [[nodiscard]] Instruction getCurrentInstruction() const override;
    [[nodiscard]] bool getIME() const override;
    [[nodiscard]] bool isHalted() const override;
    [[nodiscard]] uint64_t getCycles() const override;
    [[nodiscard]] IDebuggableReadable * getBootROM() const override;

    [[nodiscard]] uint8_t readMemoryThroughCPU(uint16_t addr) const override;
};



#endif // DEBUGGERCPU_H