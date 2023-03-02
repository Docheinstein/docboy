#ifndef DEBUGGABLECpu_H
#define DEBUGGABLECpu_H

#include "core/cpu.h"

class IDebuggableCpu {
public:
    virtual ~IDebuggableCpu() = default;

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

    [[nodiscard]] virtual Registers getRegisters() const = 0;
    [[nodiscard]] virtual Instruction getCurrentInstruction() const = 0;
    [[nodiscard]] virtual bool getIME() const = 0;
    [[nodiscard]] virtual bool isHalted() const = 0;
    [[nodiscard]] virtual uint64_t getCycles() const = 0;
};

class DebuggableCpu : public IDebuggableCpu, public Cpu {
public:
    explicit DebuggableCpu(IBus &bus);
    ~DebuggableCpu() override = default;

    [[nodiscard]] Registers getRegisters() const override;
    [[nodiscard]] Instruction getCurrentInstruction() const override;
    [[nodiscard]] bool getIME() const override;
    [[nodiscard]] bool isHalted() const override;
    [[nodiscard]] uint64_t getCycles() const override;
};

#endif // DEBUGGABLECpu_H