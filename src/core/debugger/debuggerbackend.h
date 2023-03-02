#ifndef DEBUGGERBACKEND_H
#define DEBUGGERBACKEND_H

#include <cstdint>
#include <vector>
#include <optional>
#include <memory>
#include "core/definitions.h"
#include "utils/binutils.h"
#include "core/core.h"
#include "debuggablebus.h"
#include "debuggablecpu.h"
#include "debuggablecore.h"

class IDebuggerFrontend;

class IDebuggerBackend {
public:
    enum class Command {
        Step,
        Next,
        Continue,
        Abort,
    };

    template<typename Op>
    struct Condition {
        enum class Operator {
            Equal
        };
        Operator operation;
        Op operand;
    };

    struct Breakpoint {
        uint32_t id;
        uint16_t address;
    };

    struct BreakpointHit {
        Breakpoint breakpoint;
    };

    struct Watchpoint {
        typedef IDebuggerBackend::Condition<uint8_t> Condition;
        enum class Type {
            Read,
            Write,
            ReadWrite,
            Change
        };

        uint32_t id;
        Type type;
        struct {
            uint16_t from;
            uint16_t to;
        } address;
        struct {
            bool enabled;
            Condition condition;
        } condition; // avoid to use std::optional so that this remains POD
    };

    struct WatchpointHit {
        enum class AccessType {
            Read,
            Write
        };
        Watchpoint watchpoint;
        uint16_t address;
        AccessType accessType;
        uint8_t oldValue;
        uint8_t newValue;
    };

    typedef std::vector<uint8_t> Disassemble;

    struct CpuState {
        IDebuggableCpu::Registers registers;
        IDebuggableCpu::Instruction instruction;
        bool IME;
        bool halted;
        uint64_t cycles;
    };

    struct ExecutionState {
        enum class State {
            Completed,
            BreakpointHit,
            WatchpointHit,
            Interrupted
        } state;
        union {
            WatchpointHit watchpointHit;
            BreakpointHit breakpointHit;
        };
    };

    virtual ~IDebuggerBackend() = default;

    virtual void attachFrontend(IDebuggerFrontend *frontend) = 0;
    virtual void detachFrontend() = 0;

    virtual uint32_t addBreakpoint(uint16_t addr) = 0;
    [[nodiscard]] virtual std::optional<Breakpoint> getBreakpoint(uint16_t addr) const = 0;
    [[nodiscard]] virtual const std::vector<Breakpoint> & getBreakpoints() const = 0;

    virtual uint32_t addWatchpoint(Watchpoint::Type, uint16_t from, uint16_t to, std::optional<Watchpoint::Condition>) = 0;
    [[nodiscard]] virtual std::optional<Watchpoint> getWatchpoint(uint16_t addr) const = 0;
    [[nodiscard]] virtual const std::vector<Watchpoint> & getWatchpoints() const = 0;

    virtual void removePoint(uint32_t id) = 0;
    virtual void clearPoints() = 0;

    virtual void disassemble(uint16_t addr, size_t n) = 0;
    virtual void disassembleRange(uint16_t from, uint16_t to) = 0;
    [[nodiscard]] virtual std::optional<Disassemble> getDisassembled(uint16_t addr) const = 0;

    [[nodiscard]] virtual CpuState getCpuState() const = 0;
    [[nodiscard]] virtual uint8_t readMemory(uint16_t addr) = 0;

    virtual void interrupt() = 0;
};

class DebuggerBackend : public IDebuggerBackend, public IDebuggableBus::Observer, public IDebuggableCore::Observer {
public:
    explicit DebuggerBackend(IDebuggableCore &core);
    ~DebuggerBackend() override = default;

    void attachFrontend(IDebuggerFrontend *frontend) override;
    void detachFrontend() override;

    uint32_t addBreakpoint(uint16_t addr) override;
    [[nodiscard]] std::optional<Breakpoint> getBreakpoint(uint16_t addr) const override;
    [[nodiscard]] const std::vector<Breakpoint> & getBreakpoints() const override;

    uint32_t addWatchpoint(Watchpoint::Type, uint16_t from, uint16_t to, std::optional<Watchpoint::Condition>) override;
    [[nodiscard]] std::optional<Watchpoint> getWatchpoint(uint16_t addr) const override;
    [[nodiscard]] const std::vector<Watchpoint> & getWatchpoints() const override;

    void removePoint(uint32_t id) override;
    void clearPoints() override;

    void disassemble(uint16_t addr, size_t n) override;
    void disassembleRange(uint16_t from, uint16_t to) override;
    [[nodiscard]] std::optional<Disassemble> getDisassembled(uint16_t addr) const override;

    [[nodiscard]] CpuState getCpuState() const override;
    [[nodiscard]] uint8_t readMemory(uint16_t addr) override;

    void interrupt() override;

    void onBusRead(uint16_t addr, uint8_t value) override;
    void onBusWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) override;

    bool onTick() override;

private:
    IDebuggableCpu &cpu;
    IDebuggableBus &bus;

    IDebuggerFrontend *frontend;

    std::vector<Breakpoint> breakpoints;
    std::vector<Watchpoint> watchpoints;
    std::optional<Disassemble> disassembled[0x10000];

    uint32_t nextPointId;

    std::optional<WatchpointHit> watchpointHit;

    std::optional<Command> command;

    bool interrupted;

    [[nodiscard]] std::optional<Disassemble> doDisassemble(uint16_t addr) const;
};

#endif // DEBUGGERBACKEND_H