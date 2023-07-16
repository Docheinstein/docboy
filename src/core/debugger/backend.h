#ifndef BACKEND_H
#define BACKEND_H

#include "core/core.h"
#include "core/definitions.h"
#include "cpu/cpu.h"
#include "frontend.h"
#include "ppu/ppu.h"
#include "shared.h"
#include "utils/binutils.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

class IDebuggerBackend {
public:
    virtual ~IDebuggerBackend() = default;

    virtual void attachFrontend(IDebuggerFrontend* frontend) = 0;
    virtual void detachFrontend() = 0;

    virtual uint32_t addBreakpoint(uint16_t addr) = 0;
    [[nodiscard]] virtual std::optional<Debugger::Breakpoint> getBreakpoint(uint16_t addr) const = 0;
    [[nodiscard]] virtual const std::vector<Debugger::Breakpoint>& getBreakpoints() const = 0;

    virtual uint32_t addWatchpoint(Debugger::Watchpoint::Type, uint16_t from, uint16_t to,
                                   std::optional<Debugger::Watchpoint::Condition>) = 0;
    [[nodiscard]] virtual std::optional<Debugger::Watchpoint> getWatchpoint(uint16_t addr) const = 0;
    [[nodiscard]] virtual const std::vector<Debugger::Watchpoint>& getWatchpoints() const = 0;

    virtual void removePoint(uint32_t id) = 0;
    virtual void clearPoints() = 0;

    virtual void disassemble(uint16_t addr, size_t n) = 0;
    virtual void disassembleRange(uint16_t from, uint16_t to) = 0;
    [[nodiscard]] virtual std::optional<Debugger::Disassemble> getDisassembled(uint16_t addr) const = 0;

    [[nodiscard]] virtual Debugger::CPUState getCPUState() const = 0;
    [[nodiscard]] virtual Debugger::PPUState getPPUState() const = 0;
    [[nodiscard]] virtual Debugger::LCDState getLCDState() const = 0;
    [[nodiscard]] virtual uint8_t readMemory(uint16_t addr) = 0;

    virtual void interrupt() = 0;
};

class DebuggerBackend : public IDebuggerBackend, public ICoreDebug::Observer {
public:
    explicit DebuggerBackend(ICoreDebug& core);
    ~DebuggerBackend() override = default;

    void attachFrontend(IDebuggerFrontend* frontend) override;
    void detachFrontend() override;

    uint32_t addBreakpoint(uint16_t addr) override;
    [[nodiscard]] std::optional<Debugger::Breakpoint> getBreakpoint(uint16_t addr) const override;
    [[nodiscard]] const std::vector<Debugger::Breakpoint>& getBreakpoints() const override;

    uint32_t addWatchpoint(Debugger::Watchpoint::Type, uint16_t from, uint16_t to,
                           std::optional<Debugger::Watchpoint::Condition>) override;
    [[nodiscard]] std::optional<Debugger::Watchpoint> getWatchpoint(uint16_t addr) const override;
    [[nodiscard]] const std::vector<Debugger::Watchpoint>& getWatchpoints() const override;

    void removePoint(uint32_t id) override;
    void clearPoints() override;

    void disassemble(uint16_t addr, size_t n) override;
    void disassembleRange(uint16_t from, uint16_t to) override;
    [[nodiscard]] std::optional<Debugger::Disassemble> getDisassembled(uint16_t addr) const override;

    [[nodiscard]] Debugger::CPUState getCPUState() const override;
    [[nodiscard]] Debugger::PPUState getPPUState() const override;
    [[nodiscard]] Debugger::LCDState getLCDState() const override;
    [[nodiscard]] uint8_t readMemory(uint16_t addr) override;

    void interrupt() override;

    bool onTick(uint64_t tick) override;
    void onMemoryRead(uint16_t addr, uint8_t value) override;
    void onMemoryWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) override;

private:
    ICoreDebug& core;
    IDebuggableGameBoy& gameboy;

    IDebuggerFrontend* frontend;

    std::vector<Debugger::Breakpoint> breakpoints;
    std::vector<Debugger::Watchpoint> watchpoints;
    std::optional<Debugger::Disassemble> disassembled[0x10000];

    uint32_t nextPointId;

    std::optional<Debugger::WatchpointHit> watchpointHit;

    std::optional<Debugger::Command> command;
    struct {
        uint64_t counter;

        uint64_t target;
        uint16_t stackLevel;
    } commandState;

    bool interrupted;

    [[nodiscard]] std::optional<Debugger::Disassemble> doDisassemble(uint16_t addr);
};

#endif // BACKEND_H