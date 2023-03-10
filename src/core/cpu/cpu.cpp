#include "cpu.h"
#include "utils/binutils.h"
#include "core/definitions.h"
#include "core/bus/bus.h"
#include "core/serial/port.h"

CPU::Timers::Timers(IBus &bus)
    :
    bus(bus),
    divTicks(),
    timaTicks() {

}

void CPU::Timers::tick() {
    constexpr uint32_t DIV_PERIOD = Specs::CPU::FREQUENCY / Specs::CPU::DIV_FREQUENCY;

    // DIV
    divTicks += 1;
    if (divTicks >= DIV_PERIOD) {
        divTicks = 0;
        uint8_t DIV = bus.read(Registers::Timers::DIV);
        bus.write(Registers::Timers::DIV, DIV + 1);
    }

    // TIMA
    uint8_t TAC = bus.read(Registers::Timers::TAC);
    if (get_bit<Bits::Registers::Timers::TAC::ENABLE>(TAC))
        timaTicks += 1;

    const uint32_t TIMA_PERIOD = Specs::CPU::FREQUENCY / Specs::CPU::TAC_FREQUENCY[bitmasked<2>(TAC)];
    if (timaTicks >= TIMA_PERIOD) {
        timaTicks = 0;
        uint8_t TIMA = bus.read(Registers::Timers::TIMA);
        auto [result, overflow] = sum_carry<7>(TIMA, 1);
        if (overflow) {
            uint8_t TMA = bus.read(Registers::Timers::TMA);
            bus.write(Registers::Timers::TIMA, TMA);

            // TODO: method for request interrupt
            uint8_t IF = bus.read(Registers::Interrupts::IF);
            set_bit<Bits::Interrupts::TIMER>(IF, true);
            bus.write(Registers::Interrupts::IF, IF);
        } else {
            bus.write(Registers::Timers::TIMA, result);
        }
    }
}


CPU::CPU(IBus &bus, SerialPort &serialPort, std::unique_ptr<Impl::IBootROM> bootRom) :
        bootRom(std::move(bootRom)),
        bus(bus),
        serialPort(serialPort),
        timers(bus),
        AF(), BC(), DE(), HL(), PC(), SP(),
        IME(),
        halted(),
        mCycles(),
        pendingInterrupt(),
        b(), u(), s(), uu(), lsb(), msb(), addr(),
        currentInstruction(),
        instructions {
        /* 00 */ { &CPU::NOP_m1 },
        /* 01 */ { &CPU::LD_rr_uu_m1<Register16::BC>, &CPU::LD_rr_uu_m2<Register16::BC>, &CPU::LD_rr_uu_m3<Register16::BC> },
        /* 02 */ { &CPU::LD_arr_r_m1<Register16::BC, Register8::A>, &CPU::LD_arr_r_m2<Register16::BC, Register8::A> },
        /* 03 */ { &CPU::INC_rr_m1<Register16::BC>, &CPU::INC_rr_m2<Register16::BC> },
        /* 04 */ { &CPU::INC_r_m1<Register8::B> },
        /* 05 */ { &CPU::DEC_r_m1<Register8::B> },
        /* 06 */ { &CPU::LD_r_u_m1<Register8::B>, &CPU::LD_r_u_m2<Register8::B> },
        /* 07 */ { &CPU::RLCA_m1 },
        /* 08 */ { &CPU::LD_ann_rr_m1<Register16::SP>, &CPU::LD_ann_rr_m2<Register16::SP>, &CPU::LD_ann_rr_m3<Register16::SP>, &CPU::LD_ann_rr_m4<Register16::SP>, &CPU::LD_ann_rr_m5<Register16::SP> },
        /* 09 */ { &CPU::ADD_rr_rr_m1<Register16::HL, Register16::BC>, &CPU::ADD_rr_rr_m2<Register16::HL, Register16::BC> },
        /* 0A */ { &CPU::LD_r_arr_m1<Register8::A, Register16::BC>, &CPU::LD_r_arr_m2<Register8::A, Register16::BC> },
        /* 0B */ { &CPU::DEC_rr_m1<Register16::BC>, &CPU::DEC_rr_m2<Register16::BC> },
        /* 0C */ { &CPU::INC_r_m1<Register8::C> },
        /* 0D */ { &CPU::DEC_r_m1<Register8::C> },
        /* 0E */ { &CPU::LD_r_u_m1<Register8::C>, &CPU::LD_r_u_m2<Register8::C> },
        /* 0F */ { &CPU::RRCA_m1 },
        /* 10 */ { &CPU::STOP_m1 },
        /* 11 */ { &CPU::LD_rr_uu_m1<Register16::DE>, &CPU::LD_rr_uu_m2<Register16::DE>, &CPU::LD_rr_uu_m3<Register16::DE> },
        /* 12 */ { &CPU::LD_arr_r_m1<Register16::DE, Register8::A>, &CPU::LD_arr_r_m2<Register16::DE, Register8::A> },
        /* 13 */ { &CPU::INC_rr_m1<Register16::DE>, &CPU::INC_rr_m2<Register16::DE> },
        /* 14 */ { &CPU::INC_r_m1<Register8::D> },
        /* 15 */ { &CPU::DEC_r_m1<Register8::D> },
        /* 16 */ { &CPU::LD_r_u_m1<Register8::D>, &CPU::LD_r_u_m2<Register8::D> },
        /* 17 */ { &CPU::RLA_m1 },
        /* 18 */ { &CPU::JR_s_m1, &CPU::JR_s_m2, &CPU::JR_s_m3 },
        /* 19 */ { &CPU::ADD_rr_rr_m1<Register16::HL, Register16::DE>, &CPU::ADD_rr_rr_m2<Register16::HL, Register16::DE> },
        /* 1A */ { &CPU::LD_r_arr_m1<Register8::A, Register16::DE>, &CPU::LD_r_arr_m2<Register8::A, Register16::DE> },
        /* 1B */ { &CPU::DEC_rr_m1<Register16::DE>, &CPU::DEC_rr_m2<Register16::DE> },
        /* 1C */ { &CPU::INC_r_m1<Register8::E> },
        /* 1D */ { &CPU::DEC_r_m1<Register8::E> },
        /* 1E */ { &CPU::LD_r_u_m1<Register8::E>, &CPU::LD_r_u_m2<Register8::E> },
        /* 1F */ { &CPU::RRA_m1 },
        /* 20 */ { &CPU::JR_c_s_m1<Flag::Z, false>, &CPU::JR_c_s_m2<Flag::Z, false>, &CPU::JR_c_s_m3<Flag::Z, false> },
        /* 21 */ { &CPU::LD_rr_uu_m1<Register16::HL>, &CPU::LD_rr_uu_m2<Register16::HL>, &CPU::LD_rr_uu_m3<Register16::HL> },
        /* 22 */ { &CPU::LD_arri_r_m1<Register16::HL, Register8::A, 1>, &CPU::LD_arri_r_m2<Register16::HL, Register8::A, 1> },
        /* 23 */ { &CPU::INC_rr_m1<Register16::HL>, &CPU::INC_rr_m2<Register16::HL> },
        /* 24 */ { &CPU::INC_r_m1<Register8::H> },
        /* 25 */ { &CPU::DEC_r_m1<Register8::H> },
        /* 26 */ { &CPU::LD_r_u_m1<Register8::H>, &CPU::LD_r_u_m2<Register8::H> },
        /* 27 */ { &CPU::DAA_m1 },
        /* 28 */ { &CPU::JR_c_s_m1<Flag::Z>, &CPU::JR_c_s_m2<Flag::Z>, &CPU::JR_c_s_m3<Flag::Z> },
        /* 29 */ { &CPU::ADD_rr_rr_m1<Register16::HL, Register16::HL>, &CPU::ADD_rr_rr_m2<Register16::HL, Register16::HL> },
        /* 2A */ { &CPU::LD_r_arri_m1<Register8::A, Register16::HL, 1>, &CPU::LD_r_arri_m2<Register8::A, Register16::HL, 1> },
        /* 2B */ { &CPU::DEC_rr_m1<Register16::HL>, &CPU::DEC_rr_m2<Register16::HL> },
        /* 2C */ { &CPU::INC_r_m1<Register8::L> },
        /* 2D */ { &CPU::DEC_r_m1<Register8::L> },
        /* 2E */ { &CPU::LD_r_u_m1<Register8::L>, &CPU::LD_r_u_m2<Register8::L> },
        /* 2F */ { &CPU::CPL_m1 },
        /* 30 */ { &CPU::JR_c_s_m1<Flag::C, false>, &CPU::JR_c_s_m2<Flag::C, false>, &CPU::JR_c_s_m3<Flag::C, false> },
        /* 31 */ { &CPU::LD_rr_uu_m1<Register16::SP>, &CPU::LD_rr_uu_m2<Register16::SP>, &CPU::LD_rr_uu_m3<Register16::SP> },
        /* 32 */ { &CPU::LD_arri_r_m1<Register16::HL, Register8::A, -1>, &CPU::LD_arri_r_m2<Register16::HL, Register8::A, -1> },
        /* 33 */ { &CPU::INC_rr_m1<Register16::SP>, &CPU::INC_rr_m2<Register16::SP> },
        /* 34 */ { &CPU::INC_arr_m1<Register16::HL>, &CPU::INC_arr_m2<Register16::HL>, &CPU::INC_arr_m3<Register16::HL> },
        /* 35 */ { &CPU::DEC_arr_m1<Register16::HL>, &CPU::DEC_arr_m2<Register16::HL>, &CPU::DEC_arr_m3<Register16::HL> },
        /* 36 */ { &CPU::LD_arr_u_m1<Register16::HL>, &CPU::LD_arr_u_m2<Register16::HL>, &CPU::LD_arr_u_m3<Register16::HL> },
        /* 37 */ { &CPU::SCF_m1 },
        /* 38 */ { &CPU::JR_c_s_m1<Flag::C>, &CPU::JR_c_s_m2<Flag::C>, &CPU::JR_c_s_m3<Flag::C> },
        /* 39 */ { &CPU::ADD_rr_rr_m1<Register16::HL, Register16::SP>, &CPU::ADD_rr_rr_m2<Register16::HL, Register16::SP> },
        /* 3A */ { &CPU::LD_r_arri_m1<Register8::A, Register16::HL, -1>, &CPU::LD_r_arri_m2<Register8::A, Register16::HL, -1> },
        /* 3B */ { &CPU::DEC_rr_m1<Register16::SP>, &CPU::DEC_rr_m2<Register16::SP> },
        /* 3C */ { &CPU::INC_r_m1<Register8::A> },
        /* 3D */ { &CPU::DEC_r_m1<Register8::A> },
        /* 3E */ { &CPU::LD_r_u_m1<Register8::A>, &CPU::LD_r_u_m2<Register8::A> },
        /* 3F */ { &CPU::CCF_m1 },
        /* 40 */ { &CPU::LD_r_r_m1<Register8::B, Register8::B> },
        /* 41 */ { &CPU::LD_r_r_m1<Register8::B, Register8::C> },
    //	/* 41 */ { &CPU::NOP_m1 },
        /* 42 */ { &CPU::LD_r_r_m1<Register8::B, Register8::D> },
        /* 43 */ { &CPU::LD_r_r_m1<Register8::B, Register8::E> },
        /* 44 */ { &CPU::LD_r_r_m1<Register8::B, Register8::H> },
        /* 45 */ { &CPU::LD_r_r_m1<Register8::B, Register8::L> },
        /* 46 */ { &CPU::LD_r_arr_m1<Register8::B, Register16::HL>, &CPU::LD_r_arr_m2<Register8::B, Register16::HL> },
        /* 47 */ { &CPU::LD_r_r_m1<Register8::B, Register8::A> },
        /* 48 */ { &CPU::LD_r_r_m1<Register8::C, Register8::B> },
        /* 49 */ { &CPU::LD_r_r_m1<Register8::C, Register8::C> },
        /* 4A */ { &CPU::LD_r_r_m1<Register8::C, Register8::D> },
        /* 4B */ { &CPU::LD_r_r_m1<Register8::C, Register8::E> },
        /* 4C */ { &CPU::LD_r_r_m1<Register8::C, Register8::H> },
        /* 4D */ { &CPU::LD_r_r_m1<Register8::C, Register8::L> },
        /* 4E */ { &CPU::LD_r_arr_m1<Register8::C, Register16::HL>, &CPU::LD_r_arr_m2<Register8::C, Register16::HL> },
        /* 4F */ { &CPU::LD_r_r_m1<Register8::C, Register8::A> },
        /* 50 */ { &CPU::LD_r_r_m1<Register8::D, Register8::B> },
        /* 51 */ { &CPU::LD_r_r_m1<Register8::D, Register8::C> },
        /* 52 */ { &CPU::LD_r_r_m1<Register8::D, Register8::D> },
        /* 53 */ { &CPU::LD_r_r_m1<Register8::D, Register8::E> },
        /* 54 */ { &CPU::LD_r_r_m1<Register8::D, Register8::H> },
        /* 55 */ { &CPU::LD_r_r_m1<Register8::D, Register8::L> },
        /* 56 */ { &CPU::LD_r_arr_m1<Register8::D, Register16::HL>, &CPU::LD_r_arr_m2<Register8::D, Register16::HL> },
        /* 57 */ { &CPU::LD_r_r_m1<Register8::D, Register8::A> },
        /* 58 */ { &CPU::LD_r_r_m1<Register8::E, Register8::B> },
        /* 59 */ { &CPU::LD_r_r_m1<Register8::E, Register8::C> },
        /* 5A */ { &CPU::LD_r_r_m1<Register8::E, Register8::D> },
        /* 5B */ { &CPU::LD_r_r_m1<Register8::E, Register8::E> },
        /* 5C */ { &CPU::LD_r_r_m1<Register8::E, Register8::H> },
        /* 5D */ { &CPU::LD_r_r_m1<Register8::E, Register8::L> },
        /* 5E */ { &CPU::LD_r_arr_m1<Register8::E, Register16::HL>, &CPU::LD_r_arr_m2<Register8::E, Register16::HL> },
        /* 5F */ { &CPU::LD_r_r_m1<Register8::E, Register8::A> },
        /* 60 */ { &CPU::LD_r_r_m1<Register8::H, Register8::B> },
        /* 61 */ { &CPU::LD_r_r_m1<Register8::H, Register8::C> },
        /* 62 */ { &CPU::LD_r_r_m1<Register8::H, Register8::D> },
        /* 63 */ { &CPU::LD_r_r_m1<Register8::H, Register8::E> },
        /* 64 */ { &CPU::LD_r_r_m1<Register8::H, Register8::H> },
        /* 65 */ { &CPU::LD_r_r_m1<Register8::H, Register8::L> },
        /* 66 */ { &CPU::LD_r_arr_m1<Register8::H, Register16::HL>, &CPU::LD_r_arr_m2<Register8::H, Register16::HL> },
        /* 67 */ { &CPU::LD_r_r_m1<Register8::H, Register8::A> },
        /* 68 */ { &CPU::LD_r_r_m1<Register8::L, Register8::B> },
        /* 69 */ { &CPU::LD_r_r_m1<Register8::L, Register8::C> },
        /* 6A */ { &CPU::LD_r_r_m1<Register8::L, Register8::D> },
        /* 6B */ { &CPU::LD_r_r_m1<Register8::L, Register8::E> },
        /* 6C */ { &CPU::LD_r_r_m1<Register8::L, Register8::H> },
        /* 6D */ { &CPU::LD_r_r_m1<Register8::L, Register8::L> },
        /* 6E */ { &CPU::LD_r_arr_m1<Register8::L, Register16::HL>, &CPU::LD_r_arr_m2<Register8::L, Register16::HL> },
        /* 6F */ { &CPU::LD_r_r_m1<Register8::L, Register8::A> },
        /* 70 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::B>, &CPU::LD_arr_r_m2<Register16::HL, Register8::B> },
        /* 71 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::C>, &CPU::LD_arr_r_m2<Register16::HL, Register8::C> },
        /* 72 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::D>, &CPU::LD_arr_r_m2<Register16::HL, Register8::D> },
        /* 73 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::E>, &CPU::LD_arr_r_m2<Register16::HL, Register8::E> },
        /* 74 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::H>, &CPU::LD_arr_r_m2<Register16::HL, Register8::H> },
        /* 75 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::L>, &CPU::LD_arr_r_m2<Register16::HL, Register8::L> },
        /* 76 */ { &CPU::HALT_m1 },
        /* 77 */ { &CPU::LD_arr_r_m1<Register16::HL, Register8::A>, &CPU::LD_arr_r_m2<Register16::HL, Register8::A> },
        /* 78 */ { &CPU::LD_r_r_m1<Register8::A, Register8::B> },
        /* 79 */ { &CPU::LD_r_r_m1<Register8::A, Register8::C> },
        /* 7A */ { &CPU::LD_r_r_m1<Register8::A, Register8::D> },
        /* 7B */ { &CPU::LD_r_r_m1<Register8::A, Register8::E> },
        /* 7C */ { &CPU::LD_r_r_m1<Register8::A, Register8::H> },
        /* 7D */ { &CPU::LD_r_r_m1<Register8::A, Register8::L> },
        /* 7E */ { &CPU::LD_r_arr_m1<Register8::A, Register16::HL>, &CPU::LD_r_arr_m2<Register8::A, Register16::HL> },
        /* 7F */ { &CPU::LD_r_r_m1<Register8::A, Register8::A> },
        /* 80 */ { &CPU::ADD_r_m1<Register8::B> },
        /* 81 */ { &CPU::ADD_r_m1<Register8::C> },
        /* 82 */ { &CPU::ADD_r_m1<Register8::D> },
        /* 83 */ { &CPU::ADD_r_m1<Register8::E> },
        /* 84 */ { &CPU::ADD_r_m1<Register8::H> },
        /* 85 */ { &CPU::ADD_r_m1<Register8::L> },
        /* 86 */ { &CPU::ADD_arr_m1<Register16::HL>, &CPU::ADD_arr_m2<Register16::HL> },
        /* 87 */ { &CPU::ADD_r_m1<Register8::A> },
        /* 88 */ { &CPU::ADC_r_m1<Register8::B> },
        /* 89 */ { &CPU::ADC_r_m1<Register8::C> },
        /* 8A */ { &CPU::ADC_r_m1<Register8::D> },
        /* 8B */ { &CPU::ADC_r_m1<Register8::E> },
        /* 8C */ { &CPU::ADC_r_m1<Register8::H> },
        /* 8D */ { &CPU::ADC_r_m1<Register8::L> },
        /* 8E */ { &CPU::ADC_arr_m1<Register16::HL>, &CPU::ADC_arr_m2<Register16::HL> },
        /* 8F */ { &CPU::ADC_r_m1<Register8::A> },
        /* 90 */ { &CPU::SUB_r_m1<Register8::B> },
        /* 91 */ { &CPU::SUB_r_m1<Register8::C> },
        /* 92 */ { &CPU::SUB_r_m1<Register8::D> },
        /* 93 */ { &CPU::SUB_r_m1<Register8::E> },
        /* 94 */ { &CPU::SUB_r_m1<Register8::H> },
        /* 95 */ { &CPU::SUB_r_m1<Register8::L> },
        /* 96 */ { &CPU::SUB_arr_m1<Register16::HL>, &CPU::SUB_arr_m2<Register16::HL> },
        /* 97 */ { &CPU::SUB_r_m1<Register8::A> },
        /* 98 */ { &CPU::SBC_r_m1<Register8::B> },
        /* 99 */ { &CPU::SBC_r_m1<Register8::C> },
        /* 9A */ { &CPU::SBC_r_m1<Register8::D> },
        /* 9B */ { &CPU::SBC_r_m1<Register8::E> },
        /* 9C */ { &CPU::SBC_r_m1<Register8::H> },
        /* 9D */ { &CPU::SBC_r_m1<Register8::L> },
        /* 9E */ { &CPU::SBC_arr_m1<Register16::HL>, &CPU::SBC_arr_m2<Register16::HL>  },
        /* 9F */ { &CPU::SBC_r_m1<Register8::A> },
        /* A0 */ { &CPU::AND_r_m1<Register8::B> },
        /* A1 */ { &CPU::AND_r_m1<Register8::C> },
        /* A2 */ { &CPU::AND_r_m1<Register8::D> },
        /* A3 */ { &CPU::AND_r_m1<Register8::E> },
        /* A4 */ { &CPU::AND_r_m1<Register8::H> },
        /* A5 */ { &CPU::AND_r_m1<Register8::L> },
        /* A6 */ { &CPU::AND_arr_m1<Register16::HL>, &CPU::AND_arr_m2<Register16::HL> },
        /* A7 */ { &CPU::AND_r_m1<Register8::A> },
        /* A8 */ { &CPU::XOR_r_m1<Register8::B> },
        /* A9 */ { &CPU::XOR_r_m1<Register8::C> },
        /* AA */ { &CPU::XOR_r_m1<Register8::D> },
        /* AB */ { &CPU::XOR_r_m1<Register8::E>  },
        /* AC */ { &CPU::XOR_r_m1<Register8::H>  },
        /* AD */ { &CPU::XOR_r_m1<Register8::L>  },
        /* AE */ { &CPU::XOR_arr_m1<Register16::HL>, &CPU::XOR_arr_m2<Register16::HL> },
        /* AF */ { &CPU::XOR_r_m1<Register8::A> },
        /* B0 */ { &CPU::OR_r_m1<Register8::B> },
        /* B1 */ { &CPU::OR_r_m1<Register8::C> },
        /* B2 */ { &CPU::OR_r_m1<Register8::D> },
        /* B3 */ { &CPU::OR_r_m1<Register8::E> },
        /* B4 */ { &CPU::OR_r_m1<Register8::H> },
        /* B5 */ { &CPU::OR_r_m1<Register8::L> },
        /* B6 */ { &CPU::OR_arr_m1<Register16::HL>, &CPU::OR_arr_m2<Register16::HL> },
        /* B7 */ { &CPU::OR_r_m1<Register8::A> },
        /* B8 */ { &CPU::CP_r_m1<Register8::B> },
        /* B9 */ { &CPU::CP_r_m1<Register8::C> },
        /* BA */ { &CPU::CP_r_m1<Register8::D> },
        /* BB */ { &CPU::CP_r_m1<Register8::E> },
        /* BC */ { &CPU::CP_r_m1<Register8::H> },
        /* BD */ { &CPU::CP_r_m1<Register8::L> },
        /* BE */ { &CPU::CP_arr_m1<Register16::HL>, &CPU::CP_arr_m2<Register16::HL> },
        /* BF */ { &CPU::CP_r_m1<Register8::A> },
        /* C0 */ { &CPU::RET_c_uu_m1<Flag::Z, false>, &CPU::RET_c_uu_m2<Flag::Z, false>, &CPU::RET_c_uu_m3<Flag::Z, false>,
                    &CPU::RET_c_uu_m4<Flag::Z, false>, &CPU::RET_c_uu_m5<Flag::Z, false> },
        /* C1 */ { &CPU::POP_rr_m1<Register16::BC>, &CPU::POP_rr_m2<Register16::BC>, &CPU::POP_rr_m3<Register16::BC> },
        /* C2 */ { &CPU::JP_c_uu_m1<Flag::Z, false>, &CPU::JP_c_uu_m2<Flag::Z, false>, &CPU::JP_c_uu_m3<Flag::Z, false>, &CPU::JP_c_uu_m4<Flag::Z, false> },
        /* C3 */ { &CPU::JP_uu_m1, &CPU::JP_uu_m2, &CPU::JP_uu_m3, &CPU::JP_uu_m4 },
        /* C4 */ { &CPU::CALL_c_uu_m1<Flag::Z, false>, &CPU::CALL_c_uu_m2<Flag::Z, false>, &CPU::CALL_c_uu_m3<Flag::Z, false>,
                    &CPU::CALL_c_uu_m4<Flag::Z, false>, &CPU::CALL_c_uu_m5<Flag::Z, false>, &CPU::CALL_c_uu_m6<Flag::Z, false>, },
        /* C5 */ { &CPU::PUSH_rr_m1<Register16::BC>, &CPU::PUSH_rr_m2<Register16::BC>, &CPU::PUSH_rr_m3<Register16::BC>, &CPU::PUSH_rr_m4<Register16::BC> },
        /* C6 */ { &CPU::ADD_u_m1, &CPU::ADD_u_m2 },
        /* C7 */ { &CPU::RST_m1<0x00>, &CPU::RST_m2<0x00>, &CPU::RST_m3<0x00>, &CPU::RST_m4<0x00> },
        /* C8 */ { &CPU::RET_c_uu_m1<Flag::Z>, &CPU::RET_c_uu_m2<Flag::Z>, &CPU::RET_c_uu_m3<Flag::Z>,
                    &CPU::RET_c_uu_m4<Flag::Z>, &CPU::RET_c_uu_m5<Flag::Z> },
        /* C9 */ { &CPU::RET_uu_m1, &CPU::RET_uu_m2, &CPU::RET_uu_m3, &CPU::RET_uu_m4 },
        /* CA */ { &CPU::JP_c_uu_m1<Flag::Z>, &CPU::JP_c_uu_m2<Flag::Z>, &CPU::JP_c_uu_m3<Flag::Z>, &CPU::JP_c_uu_m4<Flag::Z> },
        /* CB */ { &CPU::CB_m1 },
        /* CC */ { &CPU::CALL_c_uu_m1<Flag::Z>, &CPU::CALL_c_uu_m2<Flag::Z>, &CPU::CALL_c_uu_m3<Flag::Z>,
                    &CPU::CALL_c_uu_m4<Flag::Z>, &CPU::CALL_c_uu_m5<Flag::Z>, &CPU::CALL_c_uu_m6<Flag::Z>, },
        /* CD */ { &CPU::CALL_uu_m1, &CPU::CALL_uu_m2, &CPU::CALL_uu_m3,
                    &CPU::CALL_uu_m4, &CPU::CALL_uu_m5, &CPU::CALL_uu_m6, },
        /* CE */ { &CPU::ADC_u_m1, &CPU::ADC_u_m2 },
        /* CF */ { &CPU::RST_m1<0x08>, &CPU::RST_m2<0x08>, &CPU::RST_m3<0x08>, &CPU::RST_m4<0x08> },
        /* D0 */ { &CPU::RET_c_uu_m1<Flag::C, false>, &CPU::RET_c_uu_m2<Flag::C, false>, &CPU::RET_c_uu_m3<Flag::C, false>,
                    &CPU::RET_c_uu_m4<Flag::C, false>, &CPU::RET_c_uu_m5<Flag::C, false> },
        /* D1 */ { &CPU::POP_rr_m1<Register16::DE>, &CPU::POP_rr_m2<Register16::DE>, &CPU::POP_rr_m3<Register16::DE> },
        /* D2 */ { &CPU::JP_c_uu_m1<Flag::C, false>, &CPU::JP_c_uu_m2<Flag::C, false>, &CPU::JP_c_uu_m3<Flag::C, false>, &CPU::JP_c_uu_m4<Flag::C, false> },
        /* D3 */ { &CPU::invalidInstruction },
        /* D4 */ { &CPU::CALL_c_uu_m1<Flag::C, false>, &CPU::CALL_c_uu_m2<Flag::C, false>, &CPU::CALL_c_uu_m3<Flag::C, false>,
                    &CPU::CALL_c_uu_m4<Flag::C, false>, &CPU::CALL_c_uu_m5<Flag::C, false>, &CPU::CALL_c_uu_m6<Flag::C, false>, },
        /* D5 */ { &CPU::PUSH_rr_m1<Register16::DE>, &CPU::PUSH_rr_m2<Register16::DE>, &CPU::PUSH_rr_m3<Register16::DE>, &CPU::PUSH_rr_m4<Register16::DE> },
        /* D6 */ { &CPU::SUB_u_m1, &CPU::SUB_u_m2 },
        /* D7 */ { &CPU::RST_m1<0x10>, &CPU::RST_m2<0x10>, &CPU::RST_m3<0x10>, &CPU::RST_m4<0x10> },
        /* D8 */ { &CPU::RET_c_uu_m1<Flag::C>, &CPU::RET_c_uu_m2<Flag::C>, &CPU::RET_c_uu_m3<Flag::C>,
                    &CPU::RET_c_uu_m4<Flag::C>, &CPU::RET_c_uu_m5<Flag::C> },
        /* D9 */ { &CPU::RETI_uu_m1, &CPU::RETI_uu_m2, &CPU::RETI_uu_m3, &CPU::RETI_uu_m4 },
        /* DA */ { &CPU::JP_c_uu_m1<Flag::C>, &CPU::JP_c_uu_m2<Flag::C>, &CPU::JP_c_uu_m3<Flag::C>, &CPU::JP_c_uu_m4<Flag::C> },
        /* DB */ { &CPU::invalidInstruction },
        /* DC */ { &CPU::CALL_c_uu_m1<Flag::C>, &CPU::CALL_c_uu_m2<Flag::C>, &CPU::CALL_c_uu_m3<Flag::C>,
                    &CPU::CALL_c_uu_m4<Flag::C>, &CPU::CALL_c_uu_m5<Flag::C>, &CPU::CALL_c_uu_m6<Flag::C>, },
        /* DD */ { &CPU::invalidInstruction },
        /* DE */ { &CPU::SBC_u_m1, &CPU::SBC_u_m2 },
        /* DF */ { &CPU::RST_m1<0x18>, &CPU::RST_m2<0x18>, &CPU::RST_m3<0x18>, &CPU::RST_m4<0x18> },
        /* E0 */ { &CPU::LDH_an_r_m1<Register8::A>, &CPU::LDH_an_r_m2<Register8::A>, &CPU::LDH_an_r_m3<Register8::A> },
        /* E1 */ { &CPU::POP_rr_m1<Register16::HL>, &CPU::POP_rr_m2<Register16::HL>, &CPU::POP_rr_m3<Register16::HL> },
        /* E2 */ { &CPU::LDH_ar_r_m1<Register8::C, Register8::A>, &CPU::LDH_ar_r_m2<Register8::C, Register8::A> },
        /* E3 */ { &CPU::invalidInstruction },
        /* E4 */ { &CPU::invalidInstruction },
        /* E5 */ { &CPU::PUSH_rr_m1<Register16::HL>, &CPU::PUSH_rr_m2<Register16::HL>, &CPU::PUSH_rr_m3<Register16::HL>, &CPU::PUSH_rr_m4<Register16::HL> },
        /* E6 */ { &CPU::AND_u_m1, &CPU::AND_u_m2 },
        /* E7 */ { &CPU::RST_m1<0x20>, &CPU::RST_m2<0x20>, &CPU::RST_m3<0x20>, &CPU::RST_m4<0x20> },
        /* E8 */ { &CPU::ADD_rr_s_m1<Register16::SP>, &CPU::ADD_rr_s_m2<Register16::SP>, &CPU::ADD_rr_s_m3<Register16::SP>, &CPU::ADD_rr_s_m4<Register16::SP> },
        /* E9 */ { &CPU::JP_rr_m1<Register16::HL> },
        /* EA */ { &CPU::LD_ann_r_m1<Register8::A>, &CPU::LD_ann_r_m2<Register8::A>, &CPU::LD_ann_r_m3<Register8::A>, &CPU::LD_ann_r_m4<Register8::A> },
        /* EB */ { &CPU::invalidInstruction },
        /* EC */ { &CPU::invalidInstruction },
        /* ED */ { &CPU::invalidInstruction },
        /* EE */ { &CPU::XOR_u_m1, &CPU::XOR_u_m2 },
        /* EF */ { &CPU::RST_m1<0x28>, &CPU::RST_m2<0x28>, &CPU::RST_m3<0x28>, &CPU::RST_m4<0x28> },
        /* F0 */ { &CPU::LDH_r_an_m1<Register8::A>, &CPU::LDH_r_an_m2<Register8::A>, &CPU::LDH_r_an_m3<Register8::A> },
        /* F1 */ { &CPU::POP_rr_m1<Register16::AF>, &CPU::POP_rr_m2<Register16::AF>, &CPU::POP_rr_m3<Register16::AF> },
        /* F2 */ { &CPU::LDH_r_ar_m1<Register8::A, Register8::C>, &CPU::LDH_r_ar_m2<Register8::A, Register8::C> },
        /* F3 */ { &CPU::DI_m1 },
        /* F4 */ { &CPU::invalidInstruction },
        /* F5 */ { &CPU::PUSH_rr_m1<Register16::AF>, &CPU::PUSH_rr_m2<Register16::AF>, &CPU::PUSH_rr_m3<Register16::AF>, &CPU::PUSH_rr_m4<Register16::AF> },
        /* F6 */ { &CPU::OR_u_m1, &CPU::OR_u_m2 },
        /* F7 */ { &CPU::RST_m1<0x30>, &CPU::RST_m2<0x30>, &CPU::RST_m3<0x30>, &CPU::RST_m4<0x30> },
        /* F8 */ { &CPU::LD_rr_rrs_m1<Register16::HL, Register16::SP>, &CPU::LD_rr_rrs_m2<Register16::HL, Register16::SP>, &CPU::LD_rr_rrs_m3<Register16::HL, Register16::SP> },
        /* F9 */ { &CPU::LD_rr_rr_m1<Register16::SP, Register16::HL>, &CPU::LD_rr_rr_m2<Register16::SP, Register16::HL> },
        /* FA */ { &CPU::LD_r_ann_m1<Register8::A>, &CPU::LD_r_ann_m2<Register8::A>, &CPU::LD_r_ann_m3<Register8::A>, &CPU::LD_r_ann_m4<Register8::A> },
        /* FB */ { &CPU::EI_m1 },
        /* FC */ { &CPU::invalidInstruction },
        /* FD */ { &CPU::invalidInstruction },
        /* FE */ { &CPU::CP_u_m1, &CPU::CP_u_m2 },
        /* FF */ { &CPU::RST_m1<0x38>, &CPU::RST_m2<0x38>, &CPU::RST_m3<0x38>, &CPU::RST_m4<0x38> },
    }, instructions_cb {
        /* 00 */ { &CPU::RLC_r_m1<Register8::B> },
        /* 01 */ { &CPU::RLC_r_m1<Register8::C> },
        /* 02 */ { &CPU::RLC_r_m1<Register8::D> },
        /* 03 */ { &CPU::RLC_r_m1<Register8::E> },
        /* 04 */ { &CPU::RLC_r_m1<Register8::H> },
        /* 05 */ { &CPU::RLC_r_m1<Register8::L> },
        /* 06 */ { &CPU::RLC_arr_m1<Register16::HL>,    &CPU::RLC_arr_m2<Register16::HL>,    &CPU::RLC_arr_m3<Register16::HL> },
        /* 07 */ { &CPU::RLC_r_m1<Register8::A> },
        /* 08 */ { &CPU::RRC_r_m1<Register8::B> },
        /* 09 */ { &CPU::RRC_r_m1<Register8::C> },
        /* 0A */ { &CPU::RRC_r_m1<Register8::D> },
        /* 0B */ { &CPU::RRC_r_m1<Register8::E> },
        /* 0C */ { &CPU::RRC_r_m1<Register8::H> },
        /* 0D */ { &CPU::RRC_r_m1<Register8::L> },
        /* 0E */ { &CPU::RRC_arr_m1<Register16::HL>,    &CPU::RRC_arr_m2<Register16::HL>,    &CPU::RRC_arr_m3<Register16::HL> },
        /* 0F */ { &CPU::RRC_r_m1<Register8::A> },
        /* 10 */ { &CPU::RL_r_m1<Register8::B> },
        /* 11 */ { &CPU::RL_r_m1<Register8::C> },
        /* 12 */ { &CPU::RL_r_m1<Register8::D> },
        /* 13 */ { &CPU::RL_r_m1<Register8::E> },
        /* 14 */ { &CPU::RL_r_m1<Register8::H> },
        /* 15 */ { &CPU::RL_r_m1<Register8::L> },
        /* 16 */ { &CPU::RL_arr_m1<Register16::HL>,     &CPU::RL_arr_m2<Register16::HL>,     &CPU::RL_arr_m3<Register16::HL> },
        /* 17 */ { &CPU::RL_r_m1<Register8::A> },
        /* 18 */ { &CPU::RR_r_m1<Register8::B> },
        /* 19 */ { &CPU::RR_r_m1<Register8::C> },
        /* 1A */ { &CPU::RR_r_m1<Register8::D> },
        /* 1B */ { &CPU::RR_r_m1<Register8::E> },
        /* 1C */ { &CPU::RR_r_m1<Register8::H> },
        /* 1D */ { &CPU::RR_r_m1<Register8::L> },
        /* 1E */ { &CPU::RR_arr_m1<Register16::HL>,     &CPU::RR_arr_m2<Register16::HL>,     &CPU::RR_arr_m3<Register16::HL> },
        /* 1F */ { &CPU::RR_r_m1<Register8::A> },
        /* 20 */ { &CPU::SLA_r_m1<Register8::B> },
        /* 21 */ { &CPU::SLA_r_m1<Register8::C> },
        /* 22 */ { &CPU::SLA_r_m1<Register8::D> },
        /* 23 */ { &CPU::SLA_r_m1<Register8::E> },
        /* 24 */ { &CPU::SLA_r_m1<Register8::H> },
        /* 25 */ { &CPU::SLA_r_m1<Register8::L> },
        /* 26 */ { &CPU::SLA_arr_m1<Register16::HL>,    &CPU::SLA_arr_m2<Register16::HL>,    &CPU::SLA_arr_m3<Register16::HL>  },
        /* 27 */ { &CPU::SLA_r_m1<Register8::A> },
        /* 28 */ { &CPU::SRA_r_m1<Register8::B> },
        /* 29 */ { &CPU::SRA_r_m1<Register8::C> },
        /* 2A */ { &CPU::SRA_r_m1<Register8::D> },
        /* 2B */ { &CPU::SRA_r_m1<Register8::E> },
        /* 2C */ { &CPU::SRA_r_m1<Register8::H> },
        /* 2D */ { &CPU::SRA_r_m1<Register8::L> },
        /* 2E */ { &CPU::SRA_arr_m1<Register16::HL>,    &CPU::SRA_arr_m2<Register16::HL>,    &CPU::SRA_arr_m3<Register16::HL> },
        /* 2F */ { &CPU::SRA_r_m1<Register8::A> },
        /* 30 */ { &CPU::SWAP_r_m1<Register8::B> },
        /* 31 */ { &CPU::SWAP_r_m1<Register8::C> },
        /* 32 */ { &CPU::SWAP_r_m1<Register8::D> },
        /* 33 */ { &CPU::SWAP_r_m1<Register8::E> },
        /* 34 */ { &CPU::SWAP_r_m1<Register8::H> },
        /* 35 */ { &CPU::SWAP_r_m1<Register8::L> },
        /* 36 */ { &CPU::SWAP_arr_m1<Register16::HL>,   &CPU::SWAP_arr_m2<Register16::HL>,   &CPU::SWAP_arr_m3<Register16::HL> },
        /* 37 */ { &CPU::SWAP_r_m1<Register8::A> },
        /* 38 */ { &CPU::SRL_r_m1<Register8::B> },
        /* 39 */ { &CPU::SRL_r_m1<Register8::C> },
        /* 3A */ { &CPU::SRL_r_m1<Register8::D> },
        /* 3B */ { &CPU::SRL_r_m1<Register8::E> },
        /* 3C */ { &CPU::SRL_r_m1<Register8::H> },
        /* 3D */ { &CPU::SRL_r_m1<Register8::L> },
        /* 3E */ { &CPU::SRL_arr_m1<Register16::HL>,    &CPU::SRL_arr_m2<Register16::HL>,    &CPU::SRL_arr_m3<Register16::HL> },
        /* 3F */ { &CPU::SRL_r_m1<Register8::A> },
        /* 40 */ { &CPU::BIT_r_m1<0, Register8::B> },
        /* 41 */ { &CPU::BIT_r_m1<0, Register8::C> },
        /* 42 */ { &CPU::BIT_r_m1<0, Register8::D> },
        /* 43 */ { &CPU::BIT_r_m1<0, Register8::E> },
        /* 44 */ { &CPU::BIT_r_m1<0, Register8::H> },
        /* 45 */ { &CPU::BIT_r_m1<0, Register8::L> },
        /* 46 */ { &CPU::BIT_arr_m1<0, Register16::HL>, &CPU::BIT_arr_m2<0, Register16::HL> },
        /* 47 */ { &CPU::BIT_r_m1<0, Register8::A> },
        /* 48 */ { &CPU::BIT_r_m1<1, Register8::B> },
        /* 49 */ { &CPU::BIT_r_m1<1, Register8::C> },
        /* 4A */ { &CPU::BIT_r_m1<1, Register8::D> },
        /* 4B */ { &CPU::BIT_r_m1<1, Register8::E> },
        /* 4C */ { &CPU::BIT_r_m1<1, Register8::H> },
        /* 4D */ { &CPU::BIT_r_m1<1, Register8::L> },
        /* 4E */ { &CPU::BIT_arr_m1<1, Register16::HL>, &CPU::BIT_arr_m2<1, Register16::HL> },
        /* 4F */ { &CPU::BIT_r_m1<1, Register8::A> },
        /* 50 */ { &CPU::BIT_r_m1<2, Register8::B> },
        /* 51 */ { &CPU::BIT_r_m1<2, Register8::C> },
        /* 52 */ { &CPU::BIT_r_m1<2, Register8::D> },
        /* 53 */ { &CPU::BIT_r_m1<2, Register8::E> },
        /* 54 */ { &CPU::BIT_r_m1<2, Register8::H> },
        /* 55 */ { &CPU::BIT_r_m1<2, Register8::L> },
        /* 56 */ { &CPU::BIT_arr_m1<2, Register16::HL>, &CPU::BIT_arr_m2<2, Register16::HL> },
        /* 57 */ { &CPU::BIT_r_m1<2, Register8::A> },
        /* 58 */ { &CPU::BIT_r_m1<3, Register8::B> },
        /* 59 */ { &CPU::BIT_r_m1<3, Register8::C> },
        /* 5A */ { &CPU::BIT_r_m1<3, Register8::D> },
        /* 5B */ { &CPU::BIT_r_m1<3, Register8::E> },
        /* 5C */ { &CPU::BIT_r_m1<3, Register8::H> },
        /* 5D */ { &CPU::BIT_r_m1<3, Register8::L> },
        /* 5E */ { &CPU::BIT_arr_m1<3, Register16::HL>, &CPU::BIT_arr_m2<3, Register16::HL> },
        /* 5F */ { &CPU::BIT_r_m1<3, Register8::A> },
        /* 60 */ { &CPU::BIT_r_m1<4, Register8::B> },
        /* 61 */ { &CPU::BIT_r_m1<4, Register8::C> },
        /* 62 */ { &CPU::BIT_r_m1<4, Register8::D> },
        /* 63 */ { &CPU::BIT_r_m1<4, Register8::E> },
        /* 64 */ { &CPU::BIT_r_m1<4, Register8::H> },
        /* 65 */ { &CPU::BIT_r_m1<4, Register8::L> },
        /* 66 */ { &CPU::BIT_arr_m1<4, Register16::HL>, &CPU::BIT_arr_m2<4, Register16::HL> },
        /* 67 */ { &CPU::BIT_r_m1<4, Register8::A> },
        /* 68 */ { &CPU::BIT_r_m1<5, Register8::B> },
        /* 69 */ { &CPU::BIT_r_m1<5, Register8::C> },
        /* 6A */ { &CPU::BIT_r_m1<5, Register8::D> },
        /* 6B */ { &CPU::BIT_r_m1<5, Register8::E> },
        /* 6C */ { &CPU::BIT_r_m1<5, Register8::H> },
        /* 6D */ { &CPU::BIT_r_m1<5, Register8::L> },
        /* 6E */ { &CPU::BIT_arr_m1<5, Register16::HL>, &CPU::BIT_arr_m2<5, Register16::HL> },
        /* 6F */ { &CPU::BIT_r_m1<5, Register8::A> },
        /* 70 */ { &CPU::BIT_r_m1<6, Register8::B> },
        /* 71 */ { &CPU::BIT_r_m1<6, Register8::C> },
        /* 72 */ { &CPU::BIT_r_m1<6, Register8::D> },
        /* 73 */ { &CPU::BIT_r_m1<6, Register8::E> },
        /* 74 */ { &CPU::BIT_r_m1<6, Register8::H> },
        /* 75 */ { &CPU::BIT_r_m1<6, Register8::L> },
        /* 76 */ { &CPU::BIT_arr_m1<6, Register16::HL>, &CPU::BIT_arr_m2<6, Register16::HL> },
        /* 77 */ { &CPU::BIT_r_m1<6, Register8::A> },
        /* 78 */ { &CPU::BIT_r_m1<7, Register8::B> },
        /* 79 */ { &CPU::BIT_r_m1<7, Register8::C> },
        /* 7A */ { &CPU::BIT_r_m1<7, Register8::D> },
        /* 7B */ { &CPU::BIT_r_m1<7, Register8::E> },
        /* 7C */ { &CPU::BIT_r_m1<7, Register8::H> },
        /* 7D */ { &CPU::BIT_r_m1<7, Register8::L> },
        /* 7E */ { &CPU::BIT_arr_m1<7, Register16::HL>, &CPU::BIT_arr_m2<7, Register16::HL> },
        /* 7F */ { &CPU::BIT_r_m1<7, Register8::A> },
        /* 80 */ { &CPU::RES_r_m1<0, Register8::B> },
        /* 81 */ { &CPU::RES_r_m1<0, Register8::C> },
        /* 82 */ { &CPU::RES_r_m1<0, Register8::D> },
        /* 83 */ { &CPU::RES_r_m1<0, Register8::E> },
        /* 84 */ { &CPU::RES_r_m1<0, Register8::H> },
        /* 85 */ { &CPU::RES_r_m1<0, Register8::L> },
        /* 86 */ { &CPU::RES_arr_m1<0, Register16::HL>, &CPU::RES_arr_m2<0, Register16::HL>, &CPU::RES_arr_m3<0, Register16::HL> },
        /* 87 */ { &CPU::RES_r_m1<0, Register8::A> },
        /* 88 */ { &CPU::RES_r_m1<1, Register8::B> },
        /* 89 */ { &CPU::RES_r_m1<1, Register8::C> },
        /* 8A */ { &CPU::RES_r_m1<1, Register8::D> },
        /* 8B */ { &CPU::RES_r_m1<1, Register8::E> },
        /* 8C */ { &CPU::RES_r_m1<1, Register8::H> },
        /* 8D */ { &CPU::RES_r_m1<1, Register8::L> },
        /* 8E */ { &CPU::RES_arr_m1<1, Register16::HL>, &CPU::RES_arr_m2<1, Register16::HL>, &CPU::RES_arr_m3<1, Register16::HL> },
        /* 8F */ { &CPU::RES_r_m1<1, Register8::A> },
        /* 90 */ { &CPU::RES_r_m1<2, Register8::B> },
        /* 91 */ { &CPU::RES_r_m1<2, Register8::C> },
        /* 92 */ { &CPU::RES_r_m1<2, Register8::D> },
        /* 93 */ { &CPU::RES_r_m1<2, Register8::E> },
        /* 94 */ { &CPU::RES_r_m1<2, Register8::H> },
        /* 95 */ { &CPU::RES_r_m1<2, Register8::L> },
        /* 96 */ { &CPU::RES_arr_m1<2, Register16::HL>, &CPU::RES_arr_m2<2, Register16::HL>, &CPU::RES_arr_m3<2, Register16::HL> },
        /* 97 */ { &CPU::RES_r_m1<2, Register8::A> },
        /* 98 */ { &CPU::RES_r_m1<3, Register8::B> },
        /* 99 */ { &CPU::RES_r_m1<3, Register8::C> },
        /* 9A */ { &CPU::RES_r_m1<3, Register8::D> },
        /* 9B */ { &CPU::RES_r_m1<3, Register8::E> },
        /* 9C */ { &CPU::RES_r_m1<3, Register8::H> },
        /* 9D */ { &CPU::RES_r_m1<3, Register8::L> },
        /* 9E */ { &CPU::RES_arr_m1<3, Register16::HL>, &CPU::RES_arr_m2<3, Register16::HL>, &CPU::RES_arr_m3<3, Register16::HL> },
        /* 9F */ { &CPU::RES_r_m1<3, Register8::A> },
        /* A0 */ { &CPU::RES_r_m1<4, Register8::B> },
        /* A1 */ { &CPU::RES_r_m1<4, Register8::C> },
        /* A2 */ { &CPU::RES_r_m1<4, Register8::D> },
        /* A3 */ { &CPU::RES_r_m1<4, Register8::E> },
        /* A4 */ { &CPU::RES_r_m1<4, Register8::H> },
        /* A5 */ { &CPU::RES_r_m1<4, Register8::L> },
        /* A6 */ { &CPU::RES_arr_m1<4, Register16::HL>, &CPU::RES_arr_m2<4, Register16::HL>, &CPU::RES_arr_m3<4, Register16::HL> },
        /* A7 */ { &CPU::RES_r_m1<4, Register8::A> },
        /* A8 */ { &CPU::RES_r_m1<5, Register8::B> },
        /* A9 */ { &CPU::RES_r_m1<5, Register8::C> },
        /* AA */ { &CPU::RES_r_m1<5, Register8::D> },
        /* AB */ { &CPU::RES_r_m1<5, Register8::E> },
        /* AC */ { &CPU::RES_r_m1<5, Register8::H> },
        /* AD */ { &CPU::RES_r_m1<5, Register8::L> },
        /* AE */ { &CPU::RES_arr_m1<5, Register16::HL>, &CPU::RES_arr_m2<5, Register16::HL>, &CPU::RES_arr_m3<5, Register16::HL> },
        /* AF */ { &CPU::RES_r_m1<5, Register8::A> },
        /* B0 */ { &CPU::RES_r_m1<6, Register8::B> },
        /* B1 */ { &CPU::RES_r_m1<6, Register8::C> },
        /* B2 */ { &CPU::RES_r_m1<6, Register8::D> },
        /* B3 */ { &CPU::RES_r_m1<6, Register8::E> },
        /* B4 */ { &CPU::RES_r_m1<6, Register8::H> },
        /* B5 */ { &CPU::RES_r_m1<6, Register8::L> },
        /* B6 */ { &CPU::RES_arr_m1<6, Register16::HL>, &CPU::RES_arr_m2<6, Register16::HL>, &CPU::RES_arr_m3<6, Register16::HL> },
        /* B7 */ { &CPU::RES_r_m1<6, Register8::A> },
        /* B8 */ { &CPU::RES_r_m1<7, Register8::B> },
        /* B9 */ { &CPU::RES_r_m1<7, Register8::C> },
        /* BA */ { &CPU::RES_r_m1<7, Register8::D> },
        /* BB */ { &CPU::RES_r_m1<7, Register8::E> },
        /* BC */ { &CPU::RES_r_m1<7, Register8::H> },
        /* BD */ { &CPU::RES_r_m1<7, Register8::L> },
        /* BE */ { &CPU::RES_arr_m1<7, Register16::HL>, &CPU::RES_arr_m2<7, Register16::HL>, &CPU::RES_arr_m3<7, Register16::HL> },
        /* BF */ { &CPU::RES_r_m1<7, Register8::A> },
        /* C0 */ { &CPU::SET_r_m1<0, Register8::B> },
        /* C1 */ { &CPU::SET_r_m1<0, Register8::C> },
        /* C2 */ { &CPU::SET_r_m1<0, Register8::D> },
        /* C3 */ { &CPU::SET_r_m1<0, Register8::E> },
        /* C4 */ { &CPU::SET_r_m1<0, Register8::H> },
        /* C5 */ { &CPU::SET_r_m1<0, Register8::L> },
        /* C6 */ { &CPU::SET_arr_m1<0, Register16::HL>, &CPU::SET_arr_m2<0, Register16::HL>, &CPU::SET_arr_m3<0, Register16::HL> },
        /* C7 */ { &CPU::SET_r_m1<0, Register8::A> },
        /* C8 */ { &CPU::SET_r_m1<1, Register8::B> },
        /* C9 */ { &CPU::SET_r_m1<1, Register8::C> },
        /* CA */ { &CPU::SET_r_m1<1, Register8::D> },
        /* CB */ { &CPU::SET_r_m1<1, Register8::E> },
        /* CC */ { &CPU::SET_r_m1<1, Register8::H> },
        /* CD */ { &CPU::SET_r_m1<1, Register8::L> },
        /* CE */ { &CPU::SET_arr_m1<1, Register16::HL>, &CPU::SET_arr_m2<1, Register16::HL>, &CPU::SET_arr_m3<1, Register16::HL> },
        /* CF */ { &CPU::SET_r_m1<1, Register8::A> },
        /* D0 */ { &CPU::SET_r_m1<2, Register8::B> },
        /* D1 */ { &CPU::SET_r_m1<2, Register8::C> },
        /* D2 */ { &CPU::SET_r_m1<2, Register8::D> },
        /* D3 */ { &CPU::SET_r_m1<2, Register8::E> },
        /* D4 */ { &CPU::SET_r_m1<2, Register8::H> },
        /* D5 */ { &CPU::SET_r_m1<2, Register8::L> },
        /* D6 */ { &CPU::SET_arr_m1<2, Register16::HL>, &CPU::SET_arr_m2<2, Register16::HL>, &CPU::SET_arr_m3<2, Register16::HL> },
        /* D7 */ { &CPU::SET_r_m1<2, Register8::A> },
        /* D8 */ { &CPU::SET_r_m1<3, Register8::B> },
        /* D9 */ { &CPU::SET_r_m1<3, Register8::C> },
        /* DA */ { &CPU::SET_r_m1<3, Register8::D> },
        /* DB */ { &CPU::SET_r_m1<3, Register8::E> },
        /* DC */ { &CPU::SET_r_m1<3, Register8::H> },
        /* DD */ { &CPU::SET_r_m1<3, Register8::L> },
        /* DE */ { &CPU::SET_arr_m1<3, Register16::HL>, &CPU::SET_arr_m2<3, Register16::HL>, &CPU::SET_arr_m3<3, Register16::HL> },
        /* DF */ { &CPU::SET_r_m1<3, Register8::A> },
        /* E0 */ { &CPU::SET_r_m1<4, Register8::B> },
        /* E1 */ { &CPU::SET_r_m1<4, Register8::C> },
        /* E2 */ { &CPU::SET_r_m1<4, Register8::D> },
        /* E3 */ { &CPU::SET_r_m1<4, Register8::E> },
        /* E4 */ { &CPU::SET_r_m1<4, Register8::H> },
        /* E5 */ { &CPU::SET_r_m1<4, Register8::L> },
        /* E6 */ { &CPU::SET_arr_m1<4, Register16::HL>, &CPU::SET_arr_m2<4, Register16::HL>, &CPU::SET_arr_m3<4, Register16::HL> },
        /* E7 */ { &CPU::SET_r_m1<4, Register8::A> },
        /* E8 */ { &CPU::SET_r_m1<5, Register8::B> },
        /* E9 */ { &CPU::SET_r_m1<5, Register8::C> },
        /* EA */ { &CPU::SET_r_m1<5, Register8::D> },
        /* EB */ { &CPU::SET_r_m1<5, Register8::E> },
        /* EC */ { &CPU::SET_r_m1<5, Register8::H> },
        /* ED */ { &CPU::SET_r_m1<5, Register8::L> },
        /* EE */ { &CPU::SET_arr_m1<5, Register16::HL>, &CPU::SET_arr_m2<5, Register16::HL>, &CPU::SET_arr_m3<5, Register16::HL> },
        /* EF */ { &CPU::SET_r_m1<5, Register8::A> },
        /* F0 */ { &CPU::SET_r_m1<6, Register8::B> },
        /* F1 */ { &CPU::SET_r_m1<6, Register8::C> },
        /* F2 */ { &CPU::SET_r_m1<6, Register8::D> },
        /* F3 */ { &CPU::SET_r_m1<6, Register8::E> },
        /* F4 */ { &CPU::SET_r_m1<6, Register8::H> },
        /* F5 */ { &CPU::SET_r_m1<6, Register8::L> },
        /* F6 */ { &CPU::SET_arr_m1<6, Register16::HL>, &CPU::SET_arr_m2<6, Register16::HL>, &CPU::SET_arr_m3<6, Register16::HL> },
        /* F7 */ { &CPU::SET_r_m1<6, Register8::A> },
        /* F8 */ { &CPU::SET_r_m1<7, Register8::B> },
        /* F9 */ { &CPU::SET_r_m1<7, Register8::C> },
        /* FA */ { &CPU::SET_r_m1<7, Register8::D> },
        /* FB */ { &CPU::SET_r_m1<7, Register8::E> },
        /* FC */ { &CPU::SET_r_m1<7, Register8::H> },
        /* FD */ { &CPU::SET_r_m1<7, Register8::L> },
        /* FE */ { &CPU::SET_arr_m1<7, Register16::HL>, &CPU::SET_arr_m2<7, Register16::HL>, &CPU::SET_arr_m3<7, Register16::HL> },
        /* FF */ { &CPU::SET_r_m1<7, Register8::A> },
    }, ISR {
        /* VBLANK */ { &CPU::ISR_m1<0x40>, &CPU::ISR_m2<0x40>, &CPU::ISR_m3<0x40>, &CPU::ISR_m4<0x40>, &CPU::ISR_m5<0x40> },
        /* STAT   */ { &CPU::ISR_m1<0x48>, &CPU::ISR_m2<0x48>, &CPU::ISR_m3<0x48>, &CPU::ISR_m4<0x48>, &CPU::ISR_m5<0x48> },
        /* TIMER  */ { &CPU::ISR_m1<0x50>, &CPU::ISR_m2<0x50>, &CPU::ISR_m3<0x50>, &CPU::ISR_m4<0x50>, &CPU::ISR_m5<0x50> },
        /* SERIAL */ { &CPU::ISR_m1<0x58>, &CPU::ISR_m2<0x58>, &CPU::ISR_m3<0x58>, &CPU::ISR_m4<0x58>, &CPU::ISR_m5<0x58> },
        /* JOYPAD */ { &CPU::ISR_m1<0x60>, &CPU::ISR_m2<0x60>, &CPU::ISR_m3<0x60>, &CPU::ISR_m4<0x60>, &CPU::ISR_m5<0x60> },

    } {
    reset();
}

void CPU::reset() {
    mCycles = 0;
    currentInstruction = CurrentInstruction();
    currentInstruction.microopHandler = instructions[0]; // NOP -> fetch

    IME = false;
    b = false;
    u = 0;
    s = 0;
    uu = 0;
    lsb = 0;
    msb = 0;
    addr = 0;

    if (bootRom) {
        AF = BC = DE = HL = PC = SP = 0;
    } else {
        AF = 0x01B0;
        BC = 0x0013;
        DE = 0x00D8;
        HL = 0x014D;
        PC = 0x0100;
        SP = 0xFFFE;
    }
}

void CPU::tick() {
    // Timers
    // TODO: now? after? even if halted?
    timers.tick();

    if (halted) {
        uint8_t IE = readMemory(Registers::Interrupts::IE);
        uint8_t IF = readMemory(Registers::Interrupts::IF);
        for (uint8_t b = 0; b <= 4; b++) {
            if (get_bit(IE, b) && get_bit(IF, b)) {
                halted = false;
                // TODO: figure out if the IF bit is set to 0
                //  and if so, whether it depends on the value of IME
                writeMemory(Registers::Interrupts::IF, IF);
                if (IME) {
                    set_bit(IF, b, false);
                    pendingInterrupt = ISR[b];
                }
            }
        }
        return;
    }

    // Interrupts handling

    if (pendingInterrupt) {
        // oneliner for take the right address based on the ISR function pointer
        currentInstruction.ISR = true;
        currentInstruction.address = 0x40 + 8 * ((*pendingInterrupt - &ISR[0][0]) / 5);
        currentInstruction.microop = 0;
        currentInstruction.microopHandler = *pendingInterrupt;
        pendingInterrupt = std::nullopt;
         return; // TODO: return, else if or just if?
    }

    // TODO: else if or just if?
    else if (IME) {
        uint8_t IE = readMemory(Registers::Interrupts::IE);
        uint8_t IF = readMemory(Registers::Interrupts::IF);
        for (uint8_t b = 0; b <= 4; b++) {
            if (get_bit(IE, b) && get_bit(IF, b)) {
                set_bit(IF, b, false);
                writeMemory(Registers::Interrupts::IF, IF);
                IME = false;
                pendingInterrupt = ISR[b];
                break; // TODO: stop after first or handle them all?
            }
        }
    }

    // Microop execution

    InstructionMicroOperation micro_op = *currentInstruction.microopHandler;

    // Must be increased before execute micro op (so that fetch overwrites eventually)
    currentInstruction.microop++;
    currentInstruction.microopHandler++;

    (this->*micro_op)();

    // Serial
    uint8_t SC = bus.read(Registers::Serial::SC);
    if (get_bit<7>(SC) && get_bit<0>(SC))
        serialPort.tick();

    mCycles++;
}

uint8_t CPU::readMemory(uint16_t addr) const {
    // TODO: don't like addr < 0x100, is it always true? for every possible boot rom?
    if (bootRom && bus.read(Registers::Boot::BOOTROM) == 0 && addr < 0x100)
        return bootRom->read(addr);
    return bus.read(addr);
}

void CPU::writeMemory(uint16_t addr, uint8_t value) {
    // TODO: does the boot ROM affect writes?
    return bus.write(addr, value);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::A>() const {
    return get_byte<1>(AF);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::B>() const {
    return get_byte<1>(BC);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::C>() const {
    return get_byte<0>(BC);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::D>() const {
    return get_byte<1>(DE);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::E>() const {
    return get_byte<0>(DE);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::F>() const {
    return get_byte<0>(AF) & 0xF0; // last four bits hardwired to 0
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::H>() const {
    return get_byte<1>(HL);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::L>() const {
    return get_byte<0>(HL);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::SP_S>() const {
    return get_byte<1>(SP);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::SP_P>() const {
    return get_byte<0>(SP);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::PC_P>() const {
    return get_byte<1>(PC);
}

template<>
uint8_t CPU::readRegister8<CPU::Register8::PC_C>() const {
    return get_byte<0>(PC);
}

template<>
void CPU::writeRegister8<CPU::Register8::A>(uint8_t value) {
    set_byte<1>(AF, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::B>(uint8_t value) {
    set_byte<1>(BC, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::C>(uint8_t value) {
    set_byte<0>(BC, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::D>(uint8_t value) {
    set_byte<1>(DE, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::E>(uint8_t value) {
    set_byte<0>(DE, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::F>(uint8_t value) {
    set_byte<0>(AF, value & 0xF0);
}

template<>
void CPU::writeRegister8<CPU::Register8::H>(uint8_t value) {
    set_byte<1>(HL, value);
}

template<>
void CPU::writeRegister8<CPU::Register8::L>(uint8_t value) {
    set_byte<0>(HL, value);
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::AF>() const {
    return AF & 0xFFF0;
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::BC>() const {
    return BC;
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::DE>() const {
    return DE;
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::HL>() const {
    return HL;
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::PC>() const {
    return PC;
}

template<>
uint16_t CPU::readRegister16<CPU::Register16::SP>() const {
    return SP;
}

template<>
void CPU::writeRegister16<CPU::Register16::AF>(uint16_t value) {
    AF = value & 0xFFF0;
}

template<>
void CPU::writeRegister16<CPU::Register16::BC>(uint16_t value) {
    BC = value;
}

template<>
void CPU::writeRegister16<CPU::Register16::DE>(uint16_t value) {
    DE = value;
}

template<>
void CPU::writeRegister16<CPU::Register16::HL>(uint16_t value) {
    HL = value;
}

template<>
void CPU::writeRegister16<CPU::Register16::PC>(uint16_t value) {
    PC = value;
}

template<>
void CPU::writeRegister16<CPU::Register16::SP>(uint16_t value) {
    SP = value;
}

template<>
[[nodiscard]] bool CPU::readFlag<CPU::Flag::Z>() const {
    return get_bit<Bits::Flags::Z>(AF);
}

template<>
[[nodiscard]] bool CPU::readFlag<CPU::Flag::N>() const {
    return get_bit<Bits::Flags::N>(AF);
}

template<>
[[nodiscard]] bool CPU::readFlag<CPU::Flag::H>() const {
    return get_bit<Bits::Flags::H>(AF);
}

template<>
[[nodiscard]] bool CPU::readFlag<CPU::Flag::C>() const {
    return get_bit<Bits::Flags::C>(AF);
}

template<CPU::Flag f, bool y>
[[nodiscard]] bool CPU::checkFlag() const {
    return readFlag<f>() == y;
}

template<>
void CPU::writeFlag<CPU::Flag::Z>(bool value) {
    set_bit<Bits::Flags::Z>(AF, value);
}

template<>
void CPU::writeFlag<CPU::Flag::N>(bool value) {
    set_bit<Bits::Flags::N>(AF, value);
}

template<>
void CPU::writeFlag<CPU::Flag::H>(bool value) {
    set_bit<Bits::Flags::H>(AF, value);
}

template<>
void CPU::writeFlag<CPU::Flag::C>(bool value) {
    set_bit<Bits::Flags::C>(AF, value);
}

// ============================= INSTRUCTIONS ==================================

void CPU::fetch(bool cb) {
    currentInstruction.ISR = false;
    currentInstruction.address = PC;
    currentInstruction.microop = cb ? 1 : 0;
    if (cb)
        currentInstruction.microopHandler = instructions_cb[readMemory(PC++)];
    else
        currentInstruction.microopHandler = instructions[readMemory(PC++)];

}

void CPU::invalidInstruction() {
    throw std::runtime_error("Invalid instruction at address "+ hex(currentInstruction.address) + ")");
}

template<uint16_t nn>
void CPU::ISR_m1() {
}

template<uint16_t nn>
void CPU::ISR_m2() {
}

template<uint16_t nn>
void CPU::ISR_m3() {
    uu = PC - 1; // TODO: -1 because of prefetch but it's ugly
    writeMemory(--SP, get_byte<1>(uu));
}

template<uint16_t nn>
void CPU::ISR_m4() {
    writeMemory(--SP, get_byte<0>(uu));
}

template<uint16_t nn>
void CPU::ISR_m5() {
    PC = nn;
    fetch();
}


void CPU::NOP_m1() {
    fetch();
}

void CPU::STOP_m1() {
    // TODO: what to do here?
    fetch();
}

void CPU::HALT_m1() {
    fetch();
    halted = true;
}

void CPU::DI_m1() {
    IME = false;
    fetch();
}

void CPU::EI_m1() {
    IME = true;
    fetch();
}


// e.g. 01 | LD BC,d16

template<CPU::Register16 rr>
void CPU::LD_rr_uu_m1() {
    lsb = readMemory(PC++);
}

template<CPU::Register16 rr>
void CPU::LD_rr_uu_m2() {
    msb = readMemory(PC++);
}

template<CPU::Register16 rr>
void CPU::LD_rr_uu_m3() {
    writeRegister16<rr>(concat_bytes(msb, lsb));
    fetch();
}

// e.g. 36 | LD (HL),d8

template<CPU::Register16 rr>
void CPU::LD_arr_u_m1() {
    u = readMemory(PC++);
}

template<CPU::Register16 rr>
void CPU::LD_arr_u_m2() {
    writeMemory(readRegister16<rr>(), u);
}

template<CPU::Register16 rr>
void CPU::LD_arr_u_m3() {
    fetch();
}

// e.g. 02 | LD (BC),A

template<CPU::Register16 rr, CPU::Register8 r>
void CPU::LD_arr_r_m1() {
    writeMemory(readRegister16<rr>(), readRegister8<r>());
}

template<CPU::Register16 rr, CPU::Register8 r>
void CPU::LD_arr_r_m2() {
    fetch();
}

// e.g. 22 | LD (HL+),A

template<CPU::Register16 rr, CPU::Register8 r, int8_t inc>
void CPU::LD_arri_r_m1() {
    writeMemory(readRegister16<rr>(), readRegister8<r>());
    writeRegister16<rr>(readRegister16<rr>() + inc);
}

template<CPU::Register16 rr, CPU::Register8 r, int8_t inc>
void CPU::LD_arri_r_m2() {
    fetch();
}

// e.g. 06 | LD B,d8

template<CPU::Register8 r>
void CPU::LD_r_u_m1() {
    writeRegister8<r>(readMemory(PC++));
}

template<CPU::Register8 r>
void CPU::LD_r_u_m2() {
    fetch();
}

// e.g. 08 | LD (a16),SP

template<CPU::Register16 rr>
void CPU::LD_ann_rr_m1() {
    lsb = readMemory(PC++);
}

template<CPU::Register16 rr>
void CPU::LD_ann_rr_m2() {
    msb = readMemory(PC++);
}

template<CPU::Register16 rr>
void CPU::LD_ann_rr_m3() {
    addr = concat_bytes(msb, lsb);
    uu = readRegister16<rr>();
    writeMemory(addr, get_byte<0>(uu));
    // TODO: P or S?
}

template<CPU::Register16 rr>
void CPU::LD_ann_rr_m4() {
    writeMemory(addr + 1, get_byte<1>(uu));
    // TODO: P or S?
}

template<CPU::Register16 rr>
void CPU::LD_ann_rr_m5() {
    fetch();
}

// e.g. 0A |  LD A,(BC)

template<CPU::Register8 r, CPU::Register16 rr>
void CPU::LD_r_arr_m1() {
    writeRegister8<r>(readMemory(readRegister16<rr>()));
}

template<CPU::Register8 r, CPU::Register16 rr>
void CPU::LD_r_arr_m2() {
    fetch();
}

// e.g. 2A |  LD A,(HL+)

template<CPU::Register8 r, CPU::Register16 rr, int8_t inc>
void CPU::LD_r_arri_m1() {
    writeRegister8<r>(readMemory(readRegister16<rr>()));
    writeRegister16<rr>(readRegister16<rr>() + inc);
}

template<CPU::Register8 r, CPU::Register16 rr, int8_t inc>
void CPU::LD_r_arri_m2() {
    fetch();
}

// e.g. 41 |  LD B,C

template<CPU::Register8 r1, CPU::Register8 r2>
void CPU::LD_r_r_m1() {
    writeRegister8<r1>(readRegister8<r2>());
    fetch();
}

// e.g. E0 | LDH (a8),A

template<CPU::Register8 r>
void CPU::LDH_an_r_m1() {
    u = readMemory(PC++);
}

template<CPU::Register8 r>
void CPU::LDH_an_r_m2() {
    writeMemory(concat_bytes(0xFF, u), readRegister8<r>());
}

template<CPU::Register8 r>
void CPU::LDH_an_r_m3() {
    fetch();
}

// e.g. F0 | LDH A,(a8)

template<CPU::Register8 r>
void CPU::LDH_r_an_m1() {
    u = readMemory(PC++);
}

template<CPU::Register8 r>
void CPU::LDH_r_an_m2() {
    writeRegister8<r>(readMemory(concat_bytes(0xFF, u)));
}

template<CPU::Register8 r>
void CPU::LDH_r_an_m3() {
    fetch();
}

// e.g. E2 | LD (C),A

template<CPU::Register8 r1, CPU::Register8 r2>
void CPU::LDH_ar_r_m1() {
    writeMemory(concat_bytes(0xFF, readRegister8<r1>()), readRegister8<r2>());
}

template<CPU::Register8 r1, CPU::Register8 r2>
void CPU::LDH_ar_r_m2() {
    fetch();
}

// e.g. F2 | LD A,(C)

template<CPU::Register8 r1, CPU::Register8 r2>
void CPU::LDH_r_ar_m1() {
    writeRegister8<r1>(readMemory(concat_bytes(0xFF, readRegister8<r2>())));
}


template<CPU::Register8 r1, CPU::Register8 r2>
void CPU::LDH_r_ar_m2() {
    fetch();
}

// e.g. EA | LD (a16),A

template<CPU::Register8 r>
void CPU::LD_ann_r_m1() {
    lsb = readMemory(PC++);
}

template<CPU::Register8 r>
void CPU::LD_ann_r_m2() {
    msb = readMemory(PC++);
}

template<CPU::Register8 r>
void CPU::LD_ann_r_m3() {
    writeMemory(concat_bytes(msb, lsb), readRegister8<r>());
}

template<CPU::Register8 r>
void CPU::LD_ann_r_m4() {
    fetch();
}

// e.g. FA | LD A,(a16)

template<CPU::Register8 r>
void CPU::LD_r_ann_m1() {
    lsb = readMemory(PC++);
}

template<CPU::Register8 r>
void CPU::LD_r_ann_m2() {
    msb = readMemory(PC++);
}

template<CPU::Register8 r>
void CPU::LD_r_ann_m3() {
    writeRegister8<r>(readMemory(concat_bytes(msb, lsb)));
}

template<CPU::Register8 r>
void CPU::LD_r_ann_m4() {
    fetch();
}

// e.g. F9 | LD SP,HL

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rr_m1() {
    writeRegister16<rr1>(readRegister16<rr2>());
    // TODO: i guess this should be split in the two 8bit registers between m1 and m2
}

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rr_m2() {
    fetch();
}

// e.g. F8 | LD HL,SP+r8


template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rrs_m1() {
    s = static_cast<int8_t>(readMemory(PC++));
}

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rrs_m2() {
    uu = readRegister16<rr2>();
    auto [result, h, c] = sum_carry<3, 7>(uu, s);
    writeRegister16<rr1>(result);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    // TODO: when are flags set?
    // TODO: does it work?
}

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::LD_rr_rrs_m3() {
    fetch();
}

// e.g. 04 | INC B

template<CPU::Register8 r>
void CPU::INC_r_m1() {
    uint8_t tmp = readRegister8<r>();
    auto [result, h] = sum_carry<3>(tmp, 1);
    writeRegister8<r>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    fetch();
}


// e.g. 03 | INC BC

template<CPU::Register16 rr>
void CPU::INC_rr_m1() {
    uint16_t tmp = readRegister16<rr>();
    uint32_t result = tmp + 1;
    writeRegister16<rr>(result);
    // TODO: no flags?
}

template<CPU::Register16 rr>
void CPU::INC_rr_m2() {
    fetch();
}

// e.g. 34 | INC (HL)

template<CPU::Register16 rr>
void CPU::INC_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<CPU::Register16 rr>
void CPU::INC_arr_m2() {
    auto [result, h] = sum_carry<3>(u, 1);
    writeMemory(addr, result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
}

template<CPU::Register16 rr>
void CPU::INC_arr_m3() {
    fetch();
}

// e.g. 05 | DEC B

template<CPU::Register8 r>
void CPU::DEC_r_m1() {
    uint8_t tmp = readRegister8<r>();
    auto [result, h] = sub_borrow<3>(tmp, 1);
    writeRegister8<r>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    fetch();
}


// e.g. 0B | DEC BC

template<CPU::Register16 rr>
void CPU::DEC_rr_m1() {
    uint16_t tmp = readRegister16<rr>();
    uint32_t result = tmp - 1;
    writeRegister16<rr>(result);
    // TODO: no flags?
}

template<CPU::Register16 rr>
void CPU::DEC_rr_m2() {
    fetch();
}

// e.g. 35 | DEC (HL)

template<CPU::Register16 rr>
void CPU::DEC_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<CPU::Register16 rr>
void CPU::DEC_arr_m2() {
    auto [result, h] = sub_borrow<3>(u, 1);
    writeMemory(addr, result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
}

template<CPU::Register16 rr>
void CPU::DEC_arr_m3() {
    fetch();
}

// e.g. 80 | ADD B

template<CPU::Register8 r>
void CPU::ADD_r_m1() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 86 | ADD (HL)

template<CPU::Register16 rr>
void CPU::ADD_arr_m1() {
    u = readMemory(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::ADD_arr_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), u);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// C6 | ADD A,d8

void CPU::ADD_u_m1() {
    u = readMemory(PC++);
}

void CPU::ADD_u_m2() {
    auto [result, h, c] = sum_carry<3, 7>(readRegister8<Register8::A>(), u);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 88 | ADC B

template<CPU::Register8 r>
void CPU::ADC_r_m1() {
    // TODO: dont like this + C very much...
    auto [tmp, h0, c0] = sum_carry<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    auto [result, h, c] = sum_carry<3, 7>(tmp, readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. 8E | ADC (HL)

template<CPU::Register16 rr>
void CPU::ADC_arr_m1() {
    u = readMemory(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::ADC_arr_m2() {
    // TODO: is this ok?
    auto [tmp, h0, c0] = sum_carry<3, 7>(readRegister8<Register8::A>(), u);
    auto [result, h, c] = sum_carry<3, 7>(tmp, readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}

// CE | ADC A,d8

void CPU::ADC_u_m1() {
    u = readMemory(PC++);
}

void CPU::ADC_u_m2() {
    auto [tmp, h0, c0] = sum_carry<3, 7>(readRegister8<Register8::A>(), u);
    auto [result, h, c] = sum_carry<3, 7>(tmp, readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. 09 | ADD HL,BC

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::ADD_rr_rr_m1() {
    auto [result, h, c] = sum_carry<11, 15>(readRegister16<rr1>(), readRegister16<rr2>());
    writeRegister16<rr1>(result);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
}

template<CPU::Register16 rr1, CPU::Register16 rr2>
void CPU::ADD_rr_rr_m2() {
    fetch();
}

// e.g. E8 | ADD SP,r8

template<CPU::Register16 rr>
void CPU::ADD_rr_s_m1() {
    s = static_cast<int8_t>(readMemory(PC++));
}

template<CPU::Register16 rr>
void CPU::ADD_rr_s_m2() {
    // TODO: is it ok to carry bit 3 and 7?
    auto [result, h, c] = sum_carry<3, 7>(readRegister16<rr>(), s);
    writeRegister16<rr>(result);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
}


template<CPU::Register16 rr>
void CPU::ADD_rr_s_m3() {
    // TODO: why? i guess something about the instruction timing is wrong
}


template<CPU::Register16 rr>
void CPU::ADD_rr_s_m4() {
    fetch();
}


// e.g. 90 | SUB B

template<CPU::Register8 r>
void CPU::SUB_r_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 96 | SUB (HL)

template<CPU::Register16 rr>
void CPU::SUB_arr_m1() {
    u = readMemory(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::SUB_arr_m2() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}


// D6 | SUB A,d8

void CPU::SUB_u_m1() {
    u = readMemory(PC++);
}

void CPU::SUB_u_m2() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

// e.g. 98 | SBC B

template<CPU::Register8 r>
void CPU::SBC_r_m1() {
    // TODO: is this ok?
    auto [tmp, h0, c0] = sub_borrow<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. 9E | SBC A,(HL)

template<CPU::Register16 rr>
void CPU::SBC_arr_m1() {
    u = readMemory(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::SBC_arr_m2() {
    // TODO: dont like this - C very much...
    auto [tmp, h0, c0] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}


// D6 | SBC A,d8

void CPU::SBC_u_m1() {
    u = readMemory(PC++);
}

void CPU::SBC_u_m2() {
    auto [tmp, h0, c0] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +readFlag<Flag::C>());
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h | h0);
    writeFlag<Flag::C>(c | c0);
    fetch();
}

// e.g. A0 | AND B

template<CPU::Register8 r>
void CPU::AND_r_m1() {
    uint8_t result = readRegister8<Register8::A>() & readRegister8<r>();
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    writeFlag<Flag::C>(false);
    fetch();
}

// e.g. A6 | AND (HL)

template<CPU::Register16 rr>
void CPU::AND_arr_m1() {
    u = readMemory(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::AND_arr_m2() {
    uint8_t result = readRegister8<Register8::A>() & u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    writeFlag<Flag::C>(false);
    fetch();
}

// E6 | AND d8

void CPU::AND_u_m1() {
    u = readMemory(PC++);
}

void CPU::AND_u_m2() {
    uint8_t result = readRegister8<Register8::A>() & u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    writeFlag<Flag::C>(false);
    fetch();
}


// B0 | OR B

template<CPU::Register8 r>
void CPU::OR_r_m1() {
    uint8_t result = readRegister8<Register8::A>() | readRegister8<r>();
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

// e.g. B6 | OR (HL)

template<CPU::Register16 rr>
void CPU::OR_arr_m1() {
    u = readMemory(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::OR_arr_m2() {
    uint8_t result = readRegister8<Register8::A>() | u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}


// F6 | OR d8

void CPU::OR_u_m1() {
    u = readMemory(PC++);
}

void CPU::OR_u_m2() {
    uint8_t result = readRegister8<Register8::A>() | u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}


// e.g. A8 | XOR B

template<CPU::Register8 r>
void CPU::XOR_r_m1() {
    uint8_t result = readRegister8<Register8::A>() ^ readRegister8<r>();
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

template<CPU::Register16 rr>
void CPU::XOR_arr_m1() {
    u = readMemory(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::XOR_arr_m2() {
    uint8_t result = readRegister8<Register8::A>() ^ u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

// EE | XOR d8

void CPU::XOR_u_m1() {
    u = readMemory(PC++);
}

void CPU::XOR_u_m2() {
    uint8_t result = readRegister8<Register8::A>() ^ u;
    writeRegister8<Register8::A>(result);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}


template<CPU::Register8 r>
void CPU::CP_r_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), readRegister8<r>());
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}

template<CPU::Register16 rr>
void CPU::CP_arr_m1() {
    u = readMemory(readRegister16<rr>());
}

template<CPU::Register16 rr>
void CPU::CP_arr_m2() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}


// FE | CP d8

void CPU::CP_u_m1() {
    u = readMemory(PC++);
}

void CPU::CP_u_m2() {
    auto [result, h, c] = sub_borrow<3, 7>(readRegister8<Register8::A>(), u);
    writeFlag<Flag::Z>(result == 0);
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(h);
    writeFlag<Flag::C>(c);
    fetch();
}


// 27 | DAA

void CPU::DAA_m1() {
    u = readRegister8<Register8::A>();
    auto N = readFlag<Flag::N>();
    auto H = readFlag<Flag::H>();
    auto C = readFlag<Flag::C>();

    if (!N) {
        if (C || u > 0x99) {
            u += 0x60;
            C = true;
        }
        if (H || get_nibble<0>(u) > 0x9)
            u += 0x06;
    } else {
        if (C)
            u -= 0x60;
        if (H)
            u -= 0x06;
    }
    writeRegister8<Register8::A>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(C);
    fetch();
}

// 37 | SCF

void CPU::SCF_m1() {
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(true);
    fetch();
}

// 2F | CPL

void CPU::CPL_m1() {
    writeRegister8<Register8::A>(~readRegister8<Register8::A>());
    writeFlag<Flag::N>(true);
    writeFlag<Flag::H>(true);
    fetch();
}

// 3F | CCF_m1

void CPU::CCF_m1() {
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(!readFlag<Flag::C>());
    fetch();
}

// 07 | RLCA

void CPU::RLCA_m1() {
    u = readRegister8<Register8::A>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | b7;
    writeRegister8<Register8::A>(u);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
    fetch();
}

// 17 | RLA

void CPU::RLA_m1() {
    u = readRegister8<Register8::A>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | readFlag<Flag::C>();
    writeRegister8<Register8::A>(u);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
    fetch();
}

// 0F | RRCA

void CPU::RRCA_m1() {
    u = readRegister8<Register8::A>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (b0 << 7);
    writeRegister8<Register8::A>(u);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// 1F | RRA

void CPU::RRA_m1() {
    u = readRegister8<Register8::A>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (readFlag<Flag::C>() << 7);
    writeRegister8<Register8::A>(u);
    writeFlag<Flag::Z>(false);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// e.g. 18 | JR r8

void  CPU::JR_s_m1() {
    s = static_cast<int8_t>(readMemory(PC++));
}

void  CPU::JR_s_m2() {
    PC = (int16_t) PC + s;
}

void  CPU::JR_s_m3() {
    fetch();
}

// e.g. 28 | JR Z,r8
// e.g. 20 | JR NZ,r8

template<CPU::Flag f, bool y>
void CPU::JR_c_s_m1() {
    s = static_cast<int8_t>(readMemory(PC++));
}

template<CPU::Flag f, bool y>
void CPU::JR_c_s_m2() {
    if (checkFlag<f, y>()) {
        PC = PC + s;
    } else {
        fetch();
    }
}

template<CPU::Flag f, bool y>
void CPU::JR_c_s_m3() {
    fetch();
}



// e.g. C3 | JP a16

void  CPU::JP_uu_m1() {
    lsb = readMemory(PC++);
}

void CPU::JP_uu_m2() {
    msb = readMemory(PC++);
}

void CPU::JP_uu_m3() {
    PC = concat_bytes(msb, lsb);
}

void CPU::JP_uu_m4() {
    fetch();
}

// e.g. E9 | JP (HL)

template<CPU::Register16 rr>
void CPU::JP_rr_m1() {
    PC = readRegister16<rr>();
    fetch();
}

// e.g. CA | JP Z,a16
// e.g. C2 | JP NZ,a16

template<CPU::Flag f, bool y>
void CPU::JP_c_uu_m1() {
    lsb = readMemory(PC++);
}

template<CPU::Flag f, bool y>
void CPU::JP_c_uu_m2() {
    msb = readMemory(PC++);
}

template<CPU::Flag f, bool y>
void CPU::JP_c_uu_m3() {
    if (checkFlag<f, y>()) {
        PC = concat_bytes(msb, lsb);
    } else {
        fetch();
    }
}

template<CPU::Flag f, bool y>
void CPU::JP_c_uu_m4() {
    fetch();
}

// CD | CALL a16

void CPU::CALL_uu_m1() {
    lsb = readMemory(PC++);
}

void CPU::CALL_uu_m2() {
    msb = readMemory(PC++);
}

void CPU::CALL_uu_m3() {
    writeMemory(--SP, readRegister8<Register8::PC_P>());
}

void CPU::CALL_uu_m4() {
    writeMemory(--SP, readRegister8<Register8::PC_C>());
}

void CPU::CALL_uu_m5() {
    PC = concat_bytes(msb, lsb);
}

void CPU::CALL_uu_m6() {
    fetch();
}

// e.g. C4 | CALL NZ,a16

template<CPU::Flag f, bool y>
void CPU::CALL_c_uu_m1() {
    lsb = readMemory(PC++);
}

template<CPU::Flag f, bool y>
void CPU::CALL_c_uu_m2() {
    msb = readMemory(PC++);
}
template<CPU::Flag f, bool y>
void CPU::CALL_c_uu_m3() {
    if (checkFlag<f, y>()) {
        writeMemory(--SP, readRegister8<Register8::PC_P>());
    } else {
        fetch();
    }
}

template<CPU::Flag f, bool y>
void CPU::CALL_c_uu_m4() {
    writeMemory(--SP, readRegister8<Register8::PC_C>());
}

template<CPU::Flag f, bool y>
void CPU::CALL_c_uu_m5() {
    PC = concat_bytes(msb, lsb);
}

template<CPU::Flag f, bool y>
void CPU::CALL_c_uu_m6() {
    fetch();
}

// C7 | RST 00H

template<uint8_t n>
void CPU::RST_m1() {
    writeMemory(--SP, readRegister8<Register8::PC_P>());
}

template<uint8_t n>
void CPU::RST_m2() {
    writeMemory(--SP, readRegister8<Register8::PC_C>());
}

template<uint8_t n>
void CPU::RST_m3() {
    PC = concat_bytes(0x00, n);
}

template<uint8_t n>
void CPU::RST_m4() {
    fetch();
}

// C9 | RET

void CPU::RET_uu_m1() {
    lsb = readMemory(SP++);
}

void CPU::RET_uu_m2() {
    msb = readMemory(SP++);
}

void CPU::RET_uu_m3() {
    PC = concat_bytes(msb, lsb);
}

void CPU::RET_uu_m4() {
    fetch();
}

// D9 | RETI

void CPU::RETI_uu_m1() {
    lsb = readMemory(SP++);
}

void CPU::RETI_uu_m2() {
    msb = readMemory(SP++);
}

void CPU::RETI_uu_m3() {
    PC = concat_bytes(msb, lsb);
    IME = true;
}

void CPU::RETI_uu_m4() {
    fetch();
}

// e.g. C0 | RET NZ

template<CPU::Flag f, bool y>
void CPU::RET_c_uu_m1() {
    // TODO: really bad but don't know why this lasts 2 m cycle if false
}

template<CPU::Flag f, bool y>
void CPU::RET_c_uu_m2() {
    // TODO: really bad but don't know why this lasts 2 m cycle if false
    if (checkFlag<f, y>()) {
        lsb = readMemory(SP++);
    } else {
        fetch();
    }
}

template<CPU::Flag f, bool y>
void CPU::RET_c_uu_m3() {
    msb = readMemory(SP++);
}

template<CPU::Flag f, bool y>
void CPU::RET_c_uu_m4() {
    PC = concat_bytes(msb, lsb);
}

template<CPU::Flag f, bool y>
void CPU::RET_c_uu_m5() {
    fetch();
}

// e.g. C5 | PUSH BC

template<CPU::Register16 rr>
void CPU::PUSH_rr_m1() {
    uu = readRegister16<rr>();
}

template<CPU::Register16 rr>
void CPU::PUSH_rr_m2() {
    writeMemory(--SP, get_byte<1>(uu));
}

template<CPU::Register16 rr>
void CPU::PUSH_rr_m3() {
    writeMemory(--SP, get_byte<0>(uu));
}

template<CPU::Register16 rr>
void CPU::PUSH_rr_m4() {
    fetch();
}

// e.g. C1 | POP BC

template<CPU::Register16 rr>
void CPU::POP_rr_m1() {
    lsb = readMemory(SP++);
}

template<CPU::Register16 rr>
void CPU::POP_rr_m2() {
    msb = readMemory(SP++);
}

template<CPU::Register16 rr>
void CPU::POP_rr_m3() {
    writeRegister16<rr>(concat_bytes(msb, lsb));
    fetch();
}

void CPU::CB_m1() {
    fetch(true);
}

// e.g. e.g. CB 00 | RLC B

template<CPU::Register8 r>
void CPU::RLC_r_m1() {
    u = readRegister8<r>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | b7;
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
    fetch();
}

// e.g. e.g. CB 06 | RLC (HL)

template<CPU::Register16 rr>
void CPU::RLC_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<CPU::Register16 rr>
void CPU::RLC_arr_m2() {
    bool b7 = get_bit<7>(u);
    u = (u << 1) | b7;
    writeMemory(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
}

template<CPU::Register16 rr>
void CPU::RLC_arr_m3() {
    fetch();
}

// e.g. e.g. CB 08 | RRC B

template<CPU::Register8 r>
void CPU::RRC_r_m1() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (b0 << 7);
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// e.g. e.g. CB 0E | RRC (HL)

template<CPU::Register16 rr>
void CPU::RRC_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<CPU::Register16 rr>
void CPU::RRC_arr_m2() {
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (b0 << 7);
    writeMemory(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
}

template<CPU::Register16 rr>
void CPU::RRC_arr_m3() {
    fetch();
}


// e.g. e.g. CB 10 | RL B

template<CPU::Register8 r>
void CPU::RL_r_m1() {
    u = readRegister8<r>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | readFlag<Flag::C>();
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
    fetch();
}

// e.g. e.g. CB 16 | RL (HL)

template<CPU::Register16 rr>
void CPU::RL_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<CPU::Register16 rr>
void CPU::RL_arr_m2() {
    bool b7 = get_bit<7>(u);
    u = (u << 1) | readFlag<Flag::C>();
    writeMemory(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
}

template<CPU::Register16 rr>
void CPU::RL_arr_m3() {
    fetch();
}

// e.g. e.g. CB 18 | RR B

template<CPU::Register8 r>
void CPU::RR_r_m1() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (readFlag<Flag::C>() << 7);
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// e.g. e.g. CB 1E | RR (HL)

template<CPU::Register16 rr>
void CPU::RR_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<CPU::Register16 rr>
void CPU::RR_arr_m2() {
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (readFlag<Flag::C>() << 7);
    writeMemory(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
}

template<CPU::Register16 rr>
void CPU::RR_arr_m3() {
    fetch();
}


// e.g. CB 28 | SRA B

template<CPU::Register8 r>
void CPU::SRA_r_m1() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (u & bit<7>);
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// e.g. CB 2E | SRA (HL)

template<CPU::Register16 rr>
void CPU::SRA_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<CPU::Register16 rr>
void CPU::SRA_arr_m2() {
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (u & bit<7>);
    writeMemory(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
}

template<CPU::Register16 rr>
void CPU::SRA_arr_m3() {
    fetch();
}


// e.g. CB 38 | SRL B

template<CPU::Register8 r>
void CPU::SRL_r_m1() {
    u = readRegister8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1);
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
    fetch();
}

// e.g. CB 3E | SRL (HL)

template<CPU::Register16 rr>
void CPU::SRL_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<CPU::Register16 rr>
void CPU::SRL_arr_m2() {
    bool b0 = get_bit<0>(u);
    u = (u >> 1);
    writeMemory(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b0);
}


template<CPU::Register16 rr>
void CPU::SRL_arr_m3() {
    fetch();
}

// e.g. CB 20 | SLA B

template<CPU::Register8 r>
void CPU::SLA_r_m1() {
    u = readRegister8<r>();
    bool b7 = get_bit<7>(u);
    u = u << 1;
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
    fetch();
}

// e.g. CB 26 | SLA (HL)

template<CPU::Register16 rr>
void CPU::SLA_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<CPU::Register16 rr>
void CPU::SLA_arr_m2() {
    bool b7 = get_bit<7>(u);
    u = u << 1;
    writeMemory(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(b7);
}

template<CPU::Register16 rr>
void CPU::SLA_arr_m3() {
    fetch();
}

// e.g. CB 30 | SWAP B

template<CPU::Register8 r>
void CPU::SWAP_r_m1() {
    u = readRegister8<r>();
    u = ((u & 0x0F) << 4) | ((u & 0xF0) >> 4);
    writeRegister8<r>(u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
    fetch();
}

// e.g. CB 36 | SWAP (HL)

template<CPU::Register16 rr>
void CPU::SWAP_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<CPU::Register16 rr>
void CPU::SWAP_arr_m2() {
    u = ((u & 0x0F) << 4) | ((u & 0xF0) >> 4);
    writeMemory(addr, u);
    writeFlag<Flag::Z>(u == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(false);
    writeFlag<Flag::C>(false);
}

template<CPU::Register16 rr>
void CPU::SWAP_arr_m3() {
    fetch();
}

// e.g. CB 40 | BIT 0,B

template<uint8_t n, CPU::Register8 r>
void CPU::BIT_r_m1() {
    b = get_bit<n>(readRegister8<r>());
    writeFlag<Flag::Z>(b == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    fetch();
}

// e.g. CB 46 | BIT 0,(HL)

template<uint8_t n, CPU::Register16 rr>
void CPU::BIT_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<uint8_t n, CPU::Register16 rr>
void CPU::BIT_arr_m2() {
    b = get_bit<n>(u);
    writeFlag<Flag::Z>(b == 0);
    writeFlag<Flag::N>(false);
    writeFlag<Flag::H>(true);
    fetch();
}

// e.g. CB 80 | RES 0,B

template<uint8_t n, CPU::Register8 r>
void CPU::RES_r_m1() {
    u = readRegister8<r>();
    set_bit<n>(u, false);
    writeRegister8<r>(u);
    fetch();
}

// e.g. CB 86 | RES 0,(HL)

template<uint8_t n, CPU::Register16 rr>
void CPU::RES_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<uint8_t n, CPU::Register16 rr>
void CPU::RES_arr_m2() {
    set_bit<n>(u, false);
    writeMemory(addr, u);
}

template<uint8_t n, CPU::Register16 rr>
void CPU::RES_arr_m3() {
    fetch();
}


// e.g. CB C0 | SET 0,B

template<uint8_t n, CPU::Register8 r>
void CPU::SET_r_m1() {
    u = readRegister8<r>();
    set_bit<n>(u, true);
    writeRegister8<r>(u);
    fetch();
}

// e.g. CB C6 | SET 0,(HL)

template<uint8_t n, CPU::Register16 rr>
void CPU::SET_arr_m1() {
    addr = readRegister16<rr>();
    u = readMemory(addr);
}

template<uint8_t n, CPU::Register16 rr>
void CPU::SET_arr_m2() {
    set_bit<n>(u, true);
    writeMemory(addr, u);
}

template<uint8_t n, CPU::Register16 rr>
void CPU::SET_arr_m3() {
    fetch();
}

