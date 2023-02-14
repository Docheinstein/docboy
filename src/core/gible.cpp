#include "gible.h"
#include "log/log.h"
#include "utils/binutils.h"
#include "debugger/debuggerfrontend.h"
#include "definitions.h"
#include <vector>

Gible::Gible() :
    bus(cartridge, wram1, wram2), cpu(bus),
    debugger(),
    breakpoints() {

}

Gible::~Gible() {

}

bool Gible::loadROM(const std::string &rom) {
    DEBUG(1) << "Loading ROM: " << rom << std::endl;

    auto c = Cartridge::fromFile(rom);
    if (!c)
        return false;
    cartridge = std::move(*c);
//    if (cartridge.header().cartridge_type != 0x00)
//	    throw std::runtime_error("Cartridge type" + hex(cartridge.header().cartridge_type) + " not implemented");

#if DEBUG_LEVEL >= 1
    auto h = cartridge.header();
    DEBUG(1)
        << "ROM loaded\n"
        << "---------------\n"
        << cartridge << "\n"
        << "---------------\n"
        << "Entry point: " << hex(h.entry_point) << "\n"
        << "Title: " << h.title << "\n"
        << "CGB flag: " << hex(h.cgb_flag) << "\n"
        << "New licensee code: " << hex(h.new_licensee_code) << "\n"
        << "SGB flag: " << hex(h.sgb_flag) << "\n"
        << "Cartridge type: " << hex(h.cartridge_type) << "\n"
        << "ROM size: " << hex(h.rom_size) << "\n"
        << "RAM size: " << hex(h.ram_size) << "\n"
        << "Destination code: " << hex(h.destination_code) << "\n"
        << "Old licensee code: " << hex(h.old_licensee_code) << "\n"
        << "ROM version number: " << hex(h.rom_version_number) << "\n"
        << "Header checksum: " << hex(h.header_checksum) << "\n"
        << "Global checksum: " << hex(h.global_checksum) << "\n"
        << "---------------" << std::endl;

#endif
    return true;
}

void Gible::start() {
    while (true) {
        if (debugger) {
            do {
                if (!debugger->callback())
                    break;
            } while (true);
        } else {
            run();
        }
    }
}

void Gible::attachDebugger(DebuggerFrontend &frontend) {
    debugger = &frontend;
}

void Gible::detachDebugger() {
    debugger = nullptr;
}

void Gible::setBreakPoint(uint16_t addr) {
    breakpoints[addr] = true;
}

void Gible::unsetBreakPoint(uint16_t addr) {
    breakpoints[addr] = false;
}

bool Gible::hasBreakPoint(uint16_t addr) const {
    return breakpoints[addr];
}

void Gible::step() {
    tick();
}

void Gible::next() {
    do {
        tick();
    } while (cpu.getCurrentInstructionMicroOperation() != 0);
}

void Gible::run() {
    while (!breakpoints[cpu.getCurrentInstructionAddress()])
        tick();
}

DebuggerBackend::RegistersSnapshot Gible::getRegisters() {
    RegistersSnapshot snapshot {};
    snapshot.AF = cpu.getAF();
    snapshot.BC = cpu.getBC();
    snapshot.DE = cpu.getDE();
    snapshot.HL = cpu.getHL();
    snapshot.PC = cpu.getPC();
    snapshot.SP = cpu.getSP();
    return snapshot;
}

std::vector<DebuggerBackend::DisassembleEntry> Gible::disassemble(uint16_t from, uint16_t to) {
    std::vector<DisassembleEntry> result;
    for (uint32_t addr = from; addr <= to;) {
        DisassembleEntry entry;
        entry.address = addr;
        uint8_t opcode = bus.read(addr);
        if (opcode == 0xCB) {
            uint8_t opcodeCB = bus.read(addr + 1);
            entry.info = INSTRUCTIONS_CB[opcodeCB];
        }
        else {
            entry.info = INSTRUCTIONS[opcode];
        }

        for (uint8_t i = 0; i < entry.info.length; i++) {
            uint8_t data = bus.read(addr + i);
            entry.instruction.push_back(data);
        }

        result.push_back(entry);
        addr += entry.info.length;
    }
    return result;
}

void Gible::tick() {
    cpu.tick();
}

DebuggerBackend::Instruction Gible::getCurrentInstruction() {
    return {
        .address = cpu.getCurrentInstructionAddress(),
        .microop = cpu.getCurrentInstructionMicroOperation()
    };
}
