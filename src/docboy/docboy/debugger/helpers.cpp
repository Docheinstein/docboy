#include "docboy/debugger/helpers.h"

#include "docboy/cpu/cpu.h"
#include "docboy/gameboy/gameboy.h"

std::optional<uint8_t> DebuggerHelpers::get_isr_phase(const Cpu& cpu) {
    if (cpu.interrupt.state == Cpu::InterruptState::Pending && cpu.interrupt.remaining_ticks <= 1 &&
        cpu.instruction.microop.counter == 0 && cpu.ime == Cpu::ImeState::Enabled) {
        return 0;
    }

    if (cpu.instruction.microop.selector == &cpu.isr[0]) {
        return 0;
    }
    if (cpu.instruction.microop.selector == &cpu.isr[1]) {
        return 1;
    }
    if (cpu.instruction.microop.selector == &cpu.isr[2]) {
        return 2;
    }
    if (cpu.instruction.microop.selector == &cpu.isr[3]) {
        return 3;
    }
    if (cpu.instruction.microop.selector == &cpu.isr[4]) {
        return 4;
    }

    return std::nullopt;
}

bool DebuggerHelpers::is_in_isr(const Cpu& cpu) {
    return get_isr_phase(cpu) != std::nullopt;
}

uint8_t DebuggerHelpers::read_memory(const Mmu& mmu, uint16_t address) {
    return mmu.bus_accessors[Device::Cpu][address].read_bus(address);
}

uint8_t DebuggerHelpers::read_memory_raw(const GameBoy& gb, Bank bank, uint16_t address) {
#ifdef ENABLE_BOOTROM
    if (bank.boot) {
        return gb.boot_rom[address];
    }
#endif

    /* 0x0000 - 0x7FFF */
    if (address <= Specs::MemoryLayout::ROM1::END) {
        return gb.cartridge_slot.cartridge->read_rom_raw(bank.bank, address);
    }

    /* 0x8000 - 0x9FFF */
    if (address <= Specs::MemoryLayout::VRAM::END) {
#ifdef ENABLE_CGB
        if (bank.bank == 1) {
            return gb.vram1[address - Specs::MemoryLayout::VRAM::START];
        }
#endif
        return gb.vram0[address - Specs::MemoryLayout::VRAM::START];
    }

    /* 0xA000 - 0xBFFF */
    if (address <= Specs::MemoryLayout::RAM::END) {
        return gb.cartridge_slot.cartridge->read_ram_raw(bank.bank, address);
    }

    /* 0xC000 - 0xCFFF */
    if (address <= Specs::MemoryLayout::WRAM1::END) {
        return gb.wram1[address - Specs::MemoryLayout::WRAM1::START];
    }

    /* 0xD000 - 0xDFFF */
    if (address <= Specs::MemoryLayout::WRAM2::END) {
        return gb.wram2[bank.bank][address - Specs::MemoryLayout::WRAM2::START];
    }

    /* 0xE000 - 0xFDFF */
    if (address <= Specs::MemoryLayout::ECHO_RAM::END) {
        return gb.wram1[address - Specs::MemoryLayout::ECHO_RAM::START];
    }

    /* 0xFE00 - 0xFE9F */
    if (address <= Specs::MemoryLayout::OAM::END) {
        return gb.oam[address - Specs::MemoryLayout::OAM::START];
    }

    /* 0xFEA0 - 0xFEFF */
    if (address <= Specs::MemoryLayout::NOT_USABLE::END) {
        return gb.oam_bus.read_bus(address);
    }

    /* 0xFF00 - 0xFFFF */
    if (address <= Specs::MemoryLayout::IO::END) {
        return gb.cpu_bus.read_bus(address);
    }

    /* 0xFF80 - 0xFFFE */
    if (address <= Specs::MemoryLayout::HRAM::END) {
        return gb.hram[address - Specs::MemoryLayout::HRAM::START];
    }

    /* 0xFFFF */
    return gb.interrupts.IE;
}

Bank DebuggerHelpers::get_bank_for_address(const GameBoy& gb, uint16_t address) {
#ifdef ENABLE_BOOTROM
    if (!gb.mmu.boot_rom_locked) {
        /* 0x0000 - 0x00FF */
        if (address <= Specs::MemoryLayout::BOOTROM0::END) {
            return BootBank {};
        }
#ifdef ENABLE_CGB
        /* 0x0200 - 0x8FF */
        if (address >= Specs::MemoryLayout::BOOTROM1::START && address <= Specs::MemoryLayout::BOOTROM1::END) {
            return BootBank {};
        }
#endif
    }
#endif

    /* 0x0000 - 0x7FFF */
    if (address <= Specs::MemoryLayout::ROM1::END) {
        return NumberedBank {gb.cartridge_slot.cartridge->get_rom_bank(address)};
    }

    /* 0x8000 - 0x9FFF */
    if (address <= Specs::MemoryLayout::VRAM::END) {
#ifdef ENABLE_CGB
        return NumberedBank {gb.vram_bus.vram_bank};
#else
        return NumberedBank {0};
#endif
    }

    /* 0xA000 - 0xBFFF */
    if (address <= Specs::MemoryLayout::RAM::END) {
        return NumberedBank {gb.cartridge_slot.cartridge->get_ram_bank(address)};
    }

    /* 0xC000 - 0xCFFF */
    if (address <= Specs::MemoryLayout::WRAM1::END) {
        return NumberedBank {0};
    }

    /* 0xD000 - 0xDFFF */
    if (address <= Specs::MemoryLayout::WRAM2::END) {
#ifdef ENABLE_CGB
        return NumberedBank {gb.wram_bus.wram2_bank};
#else
        return NumberedBank {0};
#endif
    }

    /* 0xE000 - 0xFDFF */
    if (address <= Specs::MemoryLayout::ECHO_RAM::END) {
        return NumberedBank {0};
    }

    /* 0xFE00 - 0xFE9F */
    if (address <= Specs::MemoryLayout::OAM::END) {
        return NumberedBank {0};
    }

    /* 0xFEA0 - 0xFEFF */
    if (address <= Specs::MemoryLayout::NOT_USABLE::END) {
        return NumberedBank {0};
    }

    /* 0xFF00 - 0xFFFF */
    if (address <= Specs::MemoryLayout::IO::END) {
        return NumberedBank {0};
    }

    /* 0xFF80 - 0xFFFE */
    if (address <= Specs::MemoryLayout::HRAM::END) {
        return NumberedBank {0};
    }

    /* 0xFFFF */
    return NumberedBank {0};
}

bool DebuggerHelpers::is_valid_banked_address(const GameBoy& gb, Bank bank, uint16_t address) {
#ifdef ENABLE_BOOTROM
    if (bank.boot) {
        return (address <= Specs::MemoryLayout::BOOTROM0::END)
#ifdef ENABLE_CGB
               || (address >= Specs::MemoryLayout::BOOTROM1::START && address <= Specs::MemoryLayout::BOOTROM1::END)
#endif
            ;
    }
#endif

    /* 0x0000 - 0x7FFF */
    if (address <= Specs::MemoryLayout::ROM1::END) {
        return Specs::MemoryLayout::ROM0::SIZE * bank.bank < gb.cartridge_slot.cartridge->get_rom_size();
    }

    /* 0x8000 - 0x9FFF */
    if (address <= Specs::MemoryLayout::VRAM::END) {
#ifdef ENABLE_CGB
        return bank.bank <= 1;
#else
        return bank.bank == 0;
#endif
    }

    /* 0xA000 - 0xBFFF */
    if (address <= Specs::MemoryLayout::RAM::END) {
        return Specs::MemoryLayout::RAM::SIZE * bank.bank < gb.cartridge_slot.cartridge->get_ram_size();
    }

    /* 0xC000 - 0xCFFF */
    if (address <= Specs::MemoryLayout::WRAM1::END) {
        return bank.bank == 0;
    }

    /* 0xD000 - 0xDFFF */
    if (address <= Specs::MemoryLayout::WRAM2::END) {
#ifdef ENABLE_CGB
        return bank.bank < 7;
#else
        return bank.bank == 0;
#endif
    }

    /* 0xE000 - 0xFDFF */
    if (address <= Specs::MemoryLayout::ECHO_RAM::END) {
        return bank.bank == 0;
    }

    /* 0xFE00 - 0xFE9F */
    if (address <= Specs::MemoryLayout::OAM::END) {
        return bank.bank == 0;
    }

    /* 0xFEA0 - 0xFEFF */
    if (address <= Specs::MemoryLayout::NOT_USABLE::END) {
        return bank.bank == 0;
    }

    /* 0xFF00 - 0xFFFF */
    if (address <= Specs::MemoryLayout::IO::END) {
        return bank.bank == 0;
    }

    /* 0xFF80 - 0xFFFE */
    if (address <= Specs::MemoryLayout::HRAM::END) {
        return bank.bank == 0;
    }

    /* 0xFFFF */
    return bank.bank == 0;
}
