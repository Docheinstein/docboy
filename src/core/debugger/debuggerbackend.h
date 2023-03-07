#ifndef DEBUGGERBACKEND_H
#define DEBUGGERBACKEND_H

#include <cstdint>
#include <vector>
#include <optional>
#include <memory>
#include <variant>
#include "core/definitions.h"
#include "utils/binutils.h"
#include "core/core.h"
#include "debuggablecpu.h"
#include "debuggableppu.h"
#include "debuggablecore.h"

class IDebuggerFrontend;

class IDebuggerBackend {
public:
    struct CommandDot {
        uint64_t count;
    };
    struct CommandStep {
        uint64_t count;
    };
    struct CommandNext {
        uint64_t count;
    };
    struct CommandContinue {};
    struct CommandAbort {};

    typedef std::variant<
        CommandDot,
        CommandStep,
        CommandNext,
        CommandContinue,
        CommandAbort
    > Command;

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

    struct CPUState {
        IDebuggableCPU::Registers registers;
        IDebuggableCPU::Instruction instruction;
        bool IME;
        bool halted;
        uint64_t cycles;
    };

    struct PPUState {
        struct {
            IDebuggablePPU::PPUState state {};
            FIFO bgFifo;
            FIFO objFifo;
            uint32_t dots{};
        } ppu;
        struct {
            IDebuggablePPU::FetcherState state;
            uint8_t x8;
            uint8_t y;
            uint16_t lastTilemapAddr;
            uint16_t lastTileAddr;
            uint16_t lastTileDataAddr;
            uint32_t dots;
        } fetcher{};
        uint64_t cycles{};
    };

    struct LCDState {
        uint8_t x;
        uint8_t y;
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

    [[nodiscard]] virtual CPUState getCpuState() const = 0;
    [[nodiscard]] virtual PPUState getPpuState() const = 0;
    [[nodiscard]] virtual LCDState getLcdState() const = 0;
    [[nodiscard]] virtual uint8_t readMemory(uint16_t addr) = 0;

    virtual void interrupt() = 0;
};

class DebuggerBackend : public IDebuggerBackend, public IDebuggableCore::Observer {
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

    [[nodiscard]] CPUState getCpuState() const override;
    [[nodiscard]] PPUState getPpuState() const override;
    [[nodiscard]] LCDState getLcdState() const override;
    [[nodiscard]] uint8_t readMemory(uint16_t addr) override;

    void interrupt() override;

    bool onTick(uint8_t clk) override;
    void onMemoryRead(uint16_t addr, uint8_t value) override;
    void onMemoryWrite(uint16_t addr, uint8_t oldValue, uint8_t newValue) override;


private:
    IDebuggableCore &core;
    IDebuggableCPU &cpu;
    IDebuggablePPU &ppu;
    IDebuggableLCD &lcd;

    IDebuggerFrontend *frontend;

    std::vector<Breakpoint> breakpoints;
    std::vector<Watchpoint> watchpoints;
    std::optional<Disassemble> disassembled[0x10000];

    uint32_t nextPointId;

    std::optional<WatchpointHit> watchpointHit;

    std::optional<Command> command;

    bool interrupted;

    uint64_t dotCount;
    uint64_t stepCount;
    uint64_t nextCount;

    [[nodiscard]] std::optional<Disassemble> doDisassemble(uint16_t addr);
};

#endif // DEBUGGERBACKEND_H