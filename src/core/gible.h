#ifndef GIBLE_H
#define GIBLE_H

#include <string>
#include <vector>
#include <array>
#include <map>
#include <chrono>
#include "memory.h"
#include "cartridge.h"
#include "cpu.h"
#include "bus.h"
#include "debugger/debuggerbackend.h"
#include "serial/serialconsole.h"

class Gible : public DebuggerBackend, public ObservableBus::Observer, public SerialEndpoint {
public:
    Gible();
    ~Gible() override;

    bool loadROM(const std::string &rom);
    void start();

    void attachDebugger(DebuggerFrontend *frontend) override;
    void detachDebugger() override;

    void attachSerialLink(SerialLink *serialLink);

    uint32_t addBreakpoint(uint16_t addr) override;
    [[nodiscard]] std::optional<Breakpoint> getBreakpoint(uint16_t addr) const override;
    [[nodiscard]] const std::vector<Breakpoint> & getBreakpoints() const override;

    uint32_t addWatchpoint(Watchpoint::Type, uint16_t from, uint16_t to, std::optional<Watchpoint::Condition>) override;
    [[nodiscard]] std::optional<Watchpoint> getWatchpoint(uint16_t addr) const override;
    [[nodiscard]] const std::vector<Watchpoint> & getWatchpoints() const override;

    uint32_t addCyclepoint(uint64_t n) override;
    [[nodiscard]] std::optional<Cyclepoint> getCyclepoint(uint64_t n) const override;
    [[nodiscard]] const std::vector<Cyclepoint> & getCyclepoints() const override;

    uint64_t getCurrentCycle() override;
    uint64_t getCurrentMcycle() override;


    void removePoint(uint32_t id) override;
    void clearPoints() override;

//    void setBreakpoint(Breakpoint b) override;
//    void unsetBreakpoint(uint16_t addr) override;
//    [[nodiscard]] bool hasBreakpoint(uint16_t addr) const override;
//    std::optional<Breakpoint> getBreakpoint(uint16_t addr) const override;
//    void removeBreakpoint(uint32_t id) override;
//    void clearBreakpoints() override;



//    void setWatchpoint(Watchpoint w) override;
//    void unsetWatchpoint(uint16_t addr) override;
//    [[nodiscard]] bool hasWatchpoint(uint16_t addr) const override;
//    [[nodiscard]]  std::optional<Watchpoint> getWatchpoint(uint16_t addr) const override;


//    void removeWatchpoint(uint32_t id) override;
//    void clearWatchpoints() override;

    [[nodiscard]] std::optional<DisassembleEntry> getCode(uint16_t addr) const override;
    [[nodiscard]] const std::vector<DisassembleEntry> & getCode() const override;

    void disassemble(uint16_t addr, size_t n) override;
    void disassembleRange(uint16_t from, uint16_t to) override;

    Instruction getCurrentInstruction() override;
    RegistersSnapshot getRegisters() override;
    FlagsSnapshot getFlags() override;
    InterruptsSnapshot getInterrupts() override;

    uint8_t readMemory(uint16_t addr) override;

    ExecResult step() override;
    ExecResult next() override;
    ExecResult continue_() override;

    void abort() override;
    void interrupt() override;

    void reset() override;

    void onBusRead(uint16_t addr, uint8_t value) override;
    void onBusWrite(uint16_t addr, uint8_t value, uint8_t newValue) override;

    uint8_t serialRead() override;
    void serialWrite(uint8_t) override;

private:
    std::shared_ptr<Cartridge> cartridge;
    Memory<4096> wram1;
    Memory<4096> wram2;
    Memory<256> io;
    Memory<127> hram;
    Memory<1> ie;
    SerialLink *serialLink;

    ObservableBus bus;
    CPU cpu;

    DebuggerFrontend *debugger;

    bool debuggerAbortRequest;
    bool debuggerInterruptRequest;

    template<typename T, size_t size>
    struct VectorMap {
        std::vector<T> entries;
        std::map<size_t, size_t> locations;

        void set(size_t location, T value);
        void unset(size_t location);
        [[nodiscard]] bool has(size_t location) const;
        [[nodiscard]] std::optional<T> get(size_t location) const;
        void clear();
    };

//    VectorMap<Breakpoint, 0xFFFF> breakpoints;
//    VectorMap<Watchpoint, 0xFFFF> watchpoints;

    std::vector<Breakpoint> breakpoints;
    std::vector<Watchpoint> watchpoints;
    std::vector<Cyclepoint> cyclepoints;
    VectorMap<DisassembleEntry, 0xFFFF> code;
    std::optional<WatchpointTrigger> triggeredWatchpoint;

    uint64_t divCounter;
    uint64_t timaCounter;

    uint32_t nextPointId;

    ExecResult tick();
    std::optional<DisassembleEntry> doDisassemble(uint16_t addr);
};


#endif // GIBLE_H