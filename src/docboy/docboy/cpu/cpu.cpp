#include "docboy/cpu/cpu.h"
#include "docboy/bootrom/helpers.h"
#include "docboy/cpu/idu.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/joypad/joypad.h"
#include "docboy/mmu/mmu.h"
#include "docboy/stop/stopcontroller.h"

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/casts.h"
#include "utils/formatters.h"

namespace {
constexpr uint8_t STATE_INSTRUCTION_FLAG_NORMAL = 0;
constexpr uint8_t STATE_INSTRUCTION_FLAG_CB = 1;
constexpr uint8_t STATE_INSTRUCTION_FLAG_ISR = 2;
constexpr uint8_t STATE_INSTRUCTION_FLAG_NONE = 255;
} // namespace

Cpu::Cpu(Idu& idu, Interrupts& interrupts, Mmu::View<Device::Cpu> mmu, Joypad& joypad, bool& fetching, bool& halted,
         StopController& stop_controller) :
    idu {idu},
    interrupts {interrupts},
    mmu {mmu},
    joypad {joypad},
    fetching {fetching},
    halted {halted},
    stop_controller {stop_controller},
    // clang-format off
    instructions {
        /* 00 */ { &Cpu::nop_m0 },
        /* 01 */ { &Cpu::ld_rr_uu_m0<Register16::BC>, &Cpu::ld_rr_uu_m1<Register16::BC>, &Cpu::ld_rr_uu_m2<Register16::BC> },
        /* 02 */ { &Cpu::ld_arr_r_m0<Register16::BC, Register8::A>, &Cpu::ld_arr_r_m1<Register16::BC, Register8::A> },
        /* 03 */ { &Cpu::inc_rr_m0<Register16::BC>, &Cpu::inc_rr_m1<Register16::BC> },
        /* 04 */ { &Cpu::inc_r_m0<Register8::B> },
        /* 05 */ { &Cpu::dec_r_m0<Register8::B> },
        /* 06 */ { &Cpu::ld_r_u_m0<Register8::B>, &Cpu::ld_r_u_m1<Register8::B> },
        /* 07 */ { &Cpu::rlca_m0 },
        /* 08 */ { &Cpu::ld_ann_rr_m0<Register16::SP>, &Cpu::ld_ann_rr_m1<Register16::SP>, &Cpu::ld_ann_rr_m2<Register16::SP>, &Cpu::ld_ann_rr_m3<Register16::SP>, &Cpu::ld_ann_rr_m4<Register16::SP> },
        /* 09 */ { &Cpu::add_rr_rr_m0<Register16::HL, Register16::BC>, &Cpu::add_rr_rr_m1<Register16::HL, Register16::BC> },
        /* 0A */ { &Cpu::ld_r_arr_m0<Register8::A, Register16::BC>, &Cpu::ld_r_arr_m1<Register8::A, Register16::BC> },
        /* 0B */ { &Cpu::dec_rr_m0<Register16::BC>, &Cpu::dec_rr_m1<Register16::BC> },
        /* 0C */ { &Cpu::inc_r_m0<Register8::C> },
        /* 0D */ { &Cpu::dec_r_m0<Register8::C> },
        /* 0E */ { &Cpu::ld_r_u_m0<Register8::C>, &Cpu::ld_r_u_m1<Register8::C> },
        /* 0F */ { &Cpu::rrca_m0 },
        /* 10 */ { &Cpu::stop_m0 },
        /* 11 */ { &Cpu::ld_rr_uu_m0<Register16::DE>, &Cpu::ld_rr_uu_m1<Register16::DE>, &Cpu::ld_rr_uu_m2<Register16::DE> },
        /* 12 */ { &Cpu::ld_arr_r_m0<Register16::DE, Register8::A>, &Cpu::ld_arr_r_m1<Register16::DE, Register8::A> },
        /* 13 */ { &Cpu::inc_rr_m0<Register16::DE>, &Cpu::inc_rr_m1<Register16::DE> },
        /* 14 */ { &Cpu::inc_r_m0<Register8::D> },
        /* 15 */ { &Cpu::dec_r_m0<Register8::D> },
        /* 16 */ { &Cpu::ld_r_u_m0<Register8::D>, &Cpu::ld_r_u_m1<Register8::D> },
        /* 17 */ { &Cpu::rla_m0 },
        /* 18 */ { &Cpu::jr_s_m0, &Cpu::jr_s_m1, &Cpu::jr_s_m2 },
        /* 19 */ { &Cpu::add_rr_rr_m0<Register16::HL, Register16::DE>, &Cpu::add_rr_rr_m1<Register16::HL, Register16::DE> },
        /* 1A */ { &Cpu::ld_r_arr_m0<Register8::A, Register16::DE>, &Cpu::ld_r_arr_m1<Register8::A, Register16::DE> },
        /* 1B */ { &Cpu::dec_rr_m0<Register16::DE>, &Cpu::dec_rr_m1<Register16::DE> },
        /* 1C */ { &Cpu::inc_r_m0<Register8::E> },
        /* 1D */ { &Cpu::dec_r_m0<Register8::E> },
        /* 1E */ { &Cpu::ld_r_u_m0<Register8::E>, &Cpu::ld_r_u_m1<Register8::E> },
        /* 1F */ { &Cpu::rra_m0 },
        /* 20 */ { &Cpu::jr_c_s_m0<Flag::Z, false>, &Cpu::jr_c_s_m1<Flag::Z, false>, &Cpu::jr_c_s_m2<Flag::Z, false> },
        /* 21 */ { &Cpu::ld_rr_uu_m0<Register16::HL>, &Cpu::ld_rr_uu_m1<Register16::HL>, &Cpu::ld_rr_uu_m2<Register16::HL> },
        /* 22 */ { &Cpu::ld_arri_r_m0<Register16::HL, Register8::A, 1>, &Cpu::ld_arri_r_m1<Register16::HL, Register8::A, 1> },
        /* 23 */ { &Cpu::inc_rr_m0<Register16::HL>, &Cpu::inc_rr_m1<Register16::HL> },
        /* 24 */ { &Cpu::inc_r_m0<Register8::H> },
        /* 25 */ { &Cpu::dec_r_m0<Register8::H> },
        /* 26 */ { &Cpu::ld_r_u_m0<Register8::H>, &Cpu::ld_r_u_m1<Register8::H> },
        /* 27 */ { &Cpu::daa_m0 },
        /* 28 */ { &Cpu::jr_c_s_m0<Flag::Z>, &Cpu::jr_c_s_m1<Flag::Z>, &Cpu::jr_c_s_m2<Flag::Z> },
        /* 29 */ { &Cpu::add_rr_rr_m0<Register16::HL, Register16::HL>, &Cpu::add_rr_rr_m1<Register16::HL, Register16::HL> },
        /* 2A */ { &Cpu::ld_r_arri_m0<Register8::A, Register16::HL, 1>, &Cpu::ld_r_arri_m1<Register8::A, Register16::HL, 1> },
        /* 2B */ { &Cpu::dec_rr_m0<Register16::HL>, &Cpu::dec_rr_m1<Register16::HL> },
        /* 2C */ { &Cpu::inc_r_m0<Register8::L> },
        /* 2D */ { &Cpu::dec_r_m0<Register8::L> },
        /* 2E */ { &Cpu::ld_r_u_m0<Register8::L>, &Cpu::ld_r_u_m1<Register8::L> },
        /* 2F */ { &Cpu::cpl_m0 },
        /* 30 */ { &Cpu::jr_c_s_m0<Flag::C, false>, &Cpu::jr_c_s_m1<Flag::C, false>, &Cpu::jr_c_s_m2<Flag::C, false> },
        /* 31 */ { &Cpu::ld_rr_uu_m0<Register16::SP>, &Cpu::ld_rr_uu_m1<Register16::SP>, &Cpu::ld_rr_uu_m2<Register16::SP> },
        /* 32 */ { &Cpu::ld_arri_r_m0<Register16::HL, Register8::A, -1>, &Cpu::ld_arri_r_m1<Register16::HL, Register8::A, -1> },
        /* 33 */ { &Cpu::inc_rr_m0<Register16::SP>, &Cpu::inc_rr_m1<Register16::SP> },
        /* 34 */ { &Cpu::inc_arr_m0<Register16::HL>, &Cpu::inc_arr_m1<Register16::HL>, &Cpu::inc_arr_m2<Register16::HL> },
        /* 35 */ { &Cpu::dec_arr_m0<Register16::HL>, &Cpu::dec_arr_m1<Register16::HL>, &Cpu::dec_arr_m2<Register16::HL> },
        /* 36 */ { &Cpu::ld_arr_u_m0<Register16::HL>, &Cpu::ld_arr_u_m1<Register16::HL>, &Cpu::ld_arr_u_m2<Register16::HL> },
        /* 37 */ { &Cpu::scf_m0 },
        /* 38 */ { &Cpu::jr_c_s_m0<Flag::C>, &Cpu::jr_c_s_m1<Flag::C>, &Cpu::jr_c_s_m2<Flag::C> },
        /* 39 */ { &Cpu::add_rr_rr_m0<Register16::HL, Register16::SP>, &Cpu::add_rr_rr_m1<Register16::HL, Register16::SP> },
        /* 3A */ { &Cpu::ld_r_arri_m0<Register8::A, Register16::HL, -1>, &Cpu::ld_r_arri_m1<Register8::A, Register16::HL, -1> },
        /* 3B */ { &Cpu::dec_rr_m0<Register16::SP>, &Cpu::dec_rr_m1<Register16::SP> },
        /* 3C */ { &Cpu::inc_r_m0<Register8::A> },
        /* 3D */ { &Cpu::dec_r_m0<Register8::A> },
        /* 3E */ { &Cpu::ld_r_u_m0<Register8::A>, &Cpu::ld_r_u_m1<Register8::A> },
        /* 3F */ { &Cpu::ccf_m0 },
        /* 40 */ { &Cpu::ld_r_r_m0<Register8::B, Register8::B> },
        /* 41 */ { &Cpu::ld_r_r_m0<Register8::B, Register8::C> },
        /* 42 */ { &Cpu::ld_r_r_m0<Register8::B, Register8::D> },
        /* 43 */ { &Cpu::ld_r_r_m0<Register8::B, Register8::E> },
        /* 44 */ { &Cpu::ld_r_r_m0<Register8::B, Register8::H> },
        /* 45 */ { &Cpu::ld_r_r_m0<Register8::B, Register8::L> },
        /* 46 */ { &Cpu::ld_r_arr_m0<Register8::B, Register16::HL>, &Cpu::ld_r_arr_m1<Register8::B, Register16::HL> },
        /* 47 */ { &Cpu::ld_r_r_m0<Register8::B, Register8::A> },
        /* 48 */ { &Cpu::ld_r_r_m0<Register8::C, Register8::B> },
        /* 49 */ { &Cpu::ld_r_r_m0<Register8::C, Register8::C> },
        /* 4A */ { &Cpu::ld_r_r_m0<Register8::C, Register8::D> },
        /* 4B */ { &Cpu::ld_r_r_m0<Register8::C, Register8::E> },
        /* 4C */ { &Cpu::ld_r_r_m0<Register8::C, Register8::H> },
        /* 4D */ { &Cpu::ld_r_r_m0<Register8::C, Register8::L> },
        /* 4E */ { &Cpu::ld_r_arr_m0<Register8::C, Register16::HL>, &Cpu::ld_r_arr_m1<Register8::C, Register16::HL> },
        /* 4F */ { &Cpu::ld_r_r_m0<Register8::C, Register8::A> },
        /* 50 */ { &Cpu::ld_r_r_m0<Register8::D, Register8::B> },
        /* 51 */ { &Cpu::ld_r_r_m0<Register8::D, Register8::C> },
        /* 52 */ { &Cpu::ld_r_r_m0<Register8::D, Register8::D> },
        /* 53 */ { &Cpu::ld_r_r_m0<Register8::D, Register8::E> },
        /* 54 */ { &Cpu::ld_r_r_m0<Register8::D, Register8::H> },
        /* 55 */ { &Cpu::ld_r_r_m0<Register8::D, Register8::L> },
        /* 56 */ { &Cpu::ld_r_arr_m0<Register8::D, Register16::HL>, &Cpu::ld_r_arr_m1<Register8::D, Register16::HL> },
        /* 57 */ { &Cpu::ld_r_r_m0<Register8::D, Register8::A> },
        /* 58 */ { &Cpu::ld_r_r_m0<Register8::E, Register8::B> },
        /* 59 */ { &Cpu::ld_r_r_m0<Register8::E, Register8::C> },
        /* 5A */ { &Cpu::ld_r_r_m0<Register8::E, Register8::D> },
        /* 5B */ { &Cpu::ld_r_r_m0<Register8::E, Register8::E> },
        /* 5C */ { &Cpu::ld_r_r_m0<Register8::E, Register8::H> },
        /* 5D */ { &Cpu::ld_r_r_m0<Register8::E, Register8::L> },
        /* 5E */ { &Cpu::ld_r_arr_m0<Register8::E, Register16::HL>, &Cpu::ld_r_arr_m1<Register8::E, Register16::HL> },
        /* 5F */ { &Cpu::ld_r_r_m0<Register8::E, Register8::A> },
        /* 60 */ { &Cpu::ld_r_r_m0<Register8::H, Register8::B> },
        /* 61 */ { &Cpu::ld_r_r_m0<Register8::H, Register8::C> },
        /* 62 */ { &Cpu::ld_r_r_m0<Register8::H, Register8::D> },
        /* 63 */ { &Cpu::ld_r_r_m0<Register8::H, Register8::E> },
        /* 64 */ { &Cpu::ld_r_r_m0<Register8::H, Register8::H> },
        /* 65 */ { &Cpu::ld_r_r_m0<Register8::H, Register8::L> },
        /* 66 */ { &Cpu::ld_r_arr_m0<Register8::H, Register16::HL>, &Cpu::ld_r_arr_m1<Register8::H, Register16::HL> },
        /* 67 */ { &Cpu::ld_r_r_m0<Register8::H, Register8::A> },
        /* 68 */ { &Cpu::ld_r_r_m0<Register8::L, Register8::B> },
        /* 69 */ { &Cpu::ld_r_r_m0<Register8::L, Register8::C> },
        /* 6A */ { &Cpu::ld_r_r_m0<Register8::L, Register8::D> },
        /* 6B */ { &Cpu::ld_r_r_m0<Register8::L, Register8::E> },
        /* 6C */ { &Cpu::ld_r_r_m0<Register8::L, Register8::H> },
        /* 6D */ { &Cpu::ld_r_r_m0<Register8::L, Register8::L> },
        /* 6E */ { &Cpu::ld_r_arr_m0<Register8::L, Register16::HL>, &Cpu::ld_r_arr_m1<Register8::L, Register16::HL> },
        /* 6F */ { &Cpu::ld_r_r_m0<Register8::L, Register8::A> },
        /* 70 */ { &Cpu::ld_arr_r_m0<Register16::HL, Register8::B>, &Cpu::ld_arr_r_m1<Register16::HL, Register8::B> },
        /* 71 */ { &Cpu::ld_arr_r_m0<Register16::HL, Register8::C>, &Cpu::ld_arr_r_m1<Register16::HL, Register8::C> },
        /* 72 */ { &Cpu::ld_arr_r_m0<Register16::HL, Register8::D>, &Cpu::ld_arr_r_m1<Register16::HL, Register8::D> },
        /* 73 */ { &Cpu::ld_arr_r_m0<Register16::HL, Register8::E>, &Cpu::ld_arr_r_m1<Register16::HL, Register8::E> },
        /* 74 */ { &Cpu::ld_arr_r_m0<Register16::HL, Register8::H>, &Cpu::ld_arr_r_m1<Register16::HL, Register8::H> },
        /* 75 */ { &Cpu::ld_arr_r_m0<Register16::HL, Register8::L>, &Cpu::ld_arr_r_m1<Register16::HL, Register8::L> },
        /* 76 */ { &Cpu::halt_m0 },
        /* 77 */ { &Cpu::ld_arr_r_m0<Register16::HL, Register8::A>, &Cpu::ld_arr_r_m1<Register16::HL, Register8::A> },
        /* 78 */ { &Cpu::ld_r_r_m0<Register8::A, Register8::B> },
        /* 79 */ { &Cpu::ld_r_r_m0<Register8::A, Register8::C> },
        /* 7A */ { &Cpu::ld_r_r_m0<Register8::A, Register8::D> },
        /* 7B */ { &Cpu::ld_r_r_m0<Register8::A, Register8::E> },
        /* 7C */ { &Cpu::ld_r_r_m0<Register8::A, Register8::H> },
        /* 7D */ { &Cpu::ld_r_r_m0<Register8::A, Register8::L> },
        /* 7E */ { &Cpu::ld_r_arr_m0<Register8::A, Register16::HL>, &Cpu::ld_r_arr_m1<Register8::A, Register16::HL> },
        /* 7F */ { &Cpu::ld_r_r_m0<Register8::A, Register8::A> },
        /* 80 */ { &Cpu::add_r_m0<Register8::B> },
        /* 81 */ { &Cpu::add_r_m0<Register8::C> },
        /* 82 */ { &Cpu::add_r_m0<Register8::D> },
        /* 83 */ { &Cpu::add_r_m0<Register8::E> },
        /* 84 */ { &Cpu::add_r_m0<Register8::H> },
        /* 85 */ { &Cpu::add_r_m0<Register8::L> },
        /* 86 */ { &Cpu::add_arr_m0<Register16::HL>, &Cpu::add_arr_m1<Register16::HL> },
        /* 87 */ { &Cpu::add_r_m0<Register8::A> },
        /* 88 */ { &Cpu::adc_r_m0<Register8::B> },
        /* 89 */ { &Cpu::adc_r_m0<Register8::C> },
        /* 8A */ { &Cpu::adc_r_m0<Register8::D> },
        /* 8B */ { &Cpu::adc_r_m0<Register8::E> },
        /* 8C */ { &Cpu::adc_r_m0<Register8::H> },
        /* 8D */ { &Cpu::adc_r_m0<Register8::L> },
        /* 8E */ { &Cpu::adc_arr_m0<Register16::HL>, &Cpu::adc_arr_m1<Register16::HL> },
        /* 8F */ { &Cpu::adc_r_m0<Register8::A> },
        /* 90 */ { &Cpu::sub_r_m0<Register8::B> },
        /* 91 */ { &Cpu::sub_r_m0<Register8::C> },
        /* 92 */ { &Cpu::sub_r_m0<Register8::D> },
        /* 93 */ { &Cpu::sub_r_m0<Register8::E> },
        /* 94 */ { &Cpu::sub_r_m0<Register8::H> },
        /* 95 */ { &Cpu::sub_r_m0<Register8::L> },
        /* 96 */ { &Cpu::sub_arr_m0<Register16::HL>, &Cpu::sub_arr_m1<Register16::HL> },
        /* 97 */ { &Cpu::sub_r_m0<Register8::A> },
        /* 98 */ { &Cpu::sbc_r_m0<Register8::B> },
        /* 99 */ { &Cpu::sbc_r_m0<Register8::C> },
        /* 9A */ { &Cpu::sbc_r_m0<Register8::D> },
        /* 9B */ { &Cpu::sbc_r_m0<Register8::E> },
        /* 9C */ { &Cpu::sbc_r_m0<Register8::H> },
        /* 9D */ { &Cpu::sbc_r_m0<Register8::L> },
        /* 9E */ { &Cpu::sbc_arr_m0<Register16::HL>, &Cpu::sbc_arr_m1<Register16::HL>  },
        /* 9F */ { &Cpu::sbc_r_m0<Register8::A> },
        /* A0 */ { &Cpu::and_r_m0<Register8::B> },
        /* A1 */ { &Cpu::and_r_m0<Register8::C> },
        /* A2 */ { &Cpu::and_r_m0<Register8::D> },
        /* A3 */ { &Cpu::and_r_m0<Register8::E> },
        /* A4 */ { &Cpu::and_r_m0<Register8::H> },
        /* A5 */ { &Cpu::and_r_m0<Register8::L> },
        /* A6 */ { &Cpu::and_arr_m0<Register16::HL>, &Cpu::and_arr_m1<Register16::HL> },
        /* A7 */ { &Cpu::and_r_m0<Register8::A> },
        /* A8 */ { &Cpu::xor_r_m0<Register8::B> },
        /* A9 */ { &Cpu::xor_r_m0<Register8::C> },
        /* AA */ { &Cpu::xor_r_m0<Register8::D> },
        /* AB */ { &Cpu::xor_r_m0<Register8::E>  },
        /* AC */ { &Cpu::xor_r_m0<Register8::H>  },
        /* AD */ { &Cpu::xor_r_m0<Register8::L>  },
        /* AE */ { &Cpu::xor_arr_m0<Register16::HL>, &Cpu::xor_arr_m1<Register16::HL> },
        /* AF */ { &Cpu::xor_r_m0<Register8::A> },
        /* B0 */ { &Cpu::or_r_m0<Register8::B> },
        /* B1 */ { &Cpu::or_r_m0<Register8::C> },
        /* B2 */ { &Cpu::or_r_m0<Register8::D> },
        /* B3 */ { &Cpu::or_r_m0<Register8::E> },
        /* B4 */ { &Cpu::or_r_m0<Register8::H> },
        /* B5 */ { &Cpu::or_r_m0<Register8::L> },
        /* B6 */ { &Cpu::or_arr_m0<Register16::HL>, &Cpu::or_arr_m1<Register16::HL> },
        /* B7 */ { &Cpu::or_r_m0<Register8::A> },
        /* B8 */ { &Cpu::cp_r_m0<Register8::B> },
        /* B9 */ { &Cpu::cp_r_m0<Register8::C> },
        /* BA */ { &Cpu::cp_r_m0<Register8::D> },
        /* BB */ { &Cpu::cp_r_m0<Register8::E> },
        /* BC */ { &Cpu::cp_r_m0<Register8::H> },
        /* BD */ { &Cpu::cp_r_m0<Register8::L> },
        /* BE */ { &Cpu::cp_arr_m0<Register16::HL>, &Cpu::cp_arr_m1<Register16::HL> },
        /* BF */ { &Cpu::cp_r_m0<Register8::A> },
        /* C0 */ { &Cpu::ret_c_uu_m0<Flag::Z, false>, &Cpu::ret_c_uu_m1<Flag::Z, false>, &Cpu::ret_c_uu_m2<Flag::Z, false>,
                    &Cpu::ret_c_uu_m3<Flag::Z, false>, &Cpu::ret_c_uu_m4<Flag::Z, false> },
        /* C1 */ { &Cpu::pop_rr_m0<Register16::BC>, &Cpu::pop_rr_m1<Register16::BC>, &Cpu::pop_rr_m2<Register16::BC> },
        /* C2 */ { &Cpu::jp_c_uu_m0<Flag::Z, false>, &Cpu::jp_c_uu_m1<Flag::Z, false>, &Cpu::jp_c_uu_m2<Flag::Z, false>, &Cpu::jp_c_uu_m3<Flag::Z, false> },
        /* C3 */ { &Cpu::jp_uu_m0, &Cpu::jp_uu_m1, &Cpu::jp_uu_m2, &Cpu::jp_uu_m3 },
        /* C4 */ { &Cpu::call_c_uu_m0<Flag::Z, false>, &Cpu::call_c_uu_m1<Flag::Z, false>, &Cpu::call_c_uu_m2<Flag::Z, false>,
                    &Cpu::call_c_uu_m3<Flag::Z, false>, &Cpu::call_c_uu_m4<Flag::Z, false>, &Cpu::call_c_uu_m5<Flag::Z, false>, },
        /* C5 */ { &Cpu::push_rr_m0<Register16::BC>, &Cpu::push_rr_m1<Register16::BC>, &Cpu::push_rr_m2<Register16::BC>, &Cpu::push_rr_m3<Register16::BC> },
        /* C6 */ { &Cpu::add_u_m0, &Cpu::add_u_m1 },
        /* C7 */ { &Cpu::rst_m0<0x00>, &Cpu::rst_m1<0x00>, &Cpu::rst_m2<0x00>, &Cpu::rst_m3<0x00> },
        /* C8 */ { &Cpu::ret_c_uu_m0<Flag::Z>, &Cpu::ret_c_uu_m1<Flag::Z>, &Cpu::ret_c_uu_m2<Flag::Z>,
                    &Cpu::ret_c_uu_m3<Flag::Z>, &Cpu::ret_c_uu_m4<Flag::Z> },
        /* C9 */ { &Cpu::ret_uu_m0, &Cpu::ret_uu_m1, &Cpu::ret_uu_m2, &Cpu::ret_uu_m3 },
        /* CA */ { &Cpu::jp_c_uu_m0<Flag::Z>, &Cpu::jp_c_uu_m1<Flag::Z>, &Cpu::jp_c_uu_m2<Flag::Z>, &Cpu::jp_c_uu_m3<Flag::Z> },
        /* CB */ { &Cpu::cb_m0 },
        /* CC */ { &Cpu::call_c_uu_m0<Flag::Z>, &Cpu::call_c_uu_m1<Flag::Z>, &Cpu::call_c_uu_m2<Flag::Z>,
                    &Cpu::call_c_uu_m3<Flag::Z>, &Cpu::call_c_uu_m4<Flag::Z>, &Cpu::call_c_uu_m5<Flag::Z>, },
        /* CD */ { &Cpu::call_uu_m0, &Cpu::call_uu_m1, &Cpu::call_uu_m2,
                    &Cpu::call_uu_m3, &Cpu::call_uu_m4, &Cpu::call_uu_m5, },
        /* CE */ { &Cpu::adc_u_m0, &Cpu::adc_u_m1 },
        /* CF */ { &Cpu::rst_m0<0x08>, &Cpu::rst_m1<0x08>, &Cpu::rst_m2<0x08>, &Cpu::rst_m3<0x08> },
        /* D0 */ { &Cpu::ret_c_uu_m0<Flag::C, false>, &Cpu::ret_c_uu_m1<Flag::C, false>, &Cpu::ret_c_uu_m2<Flag::C, false>,
                    &Cpu::ret_c_uu_m3<Flag::C, false>, &Cpu::ret_c_uu_m4<Flag::C, false> },
        /* D1 */ { &Cpu::pop_rr_m0<Register16::DE>, &Cpu::pop_rr_m1<Register16::DE>, &Cpu::pop_rr_m2<Register16::DE> },
        /* D2 */ { &Cpu::jp_c_uu_m0<Flag::C, false>, &Cpu::jp_c_uu_m1<Flag::C, false>, &Cpu::jp_c_uu_m2<Flag::C, false>, &Cpu::jp_c_uu_m3<Flag::C, false> },
        /* D3 */ { &Cpu::invalid_microop },
        /* D4 */ { &Cpu::call_c_uu_m0<Flag::C, false>, &Cpu::call_c_uu_m1<Flag::C, false>, &Cpu::call_c_uu_m2<Flag::C, false>,
                    &Cpu::call_c_uu_m3<Flag::C, false>, &Cpu::call_c_uu_m4<Flag::C, false>, &Cpu::call_c_uu_m5<Flag::C, false>, },
        /* D5 */ { &Cpu::push_rr_m0<Register16::DE>, &Cpu::push_rr_m1<Register16::DE>, &Cpu::push_rr_m2<Register16::DE>, &Cpu::push_rr_m3<Register16::DE> },
        /* D6 */ { &Cpu::sub_u_m0, &Cpu::sub_u_m1 },
        /* D7 */ { &Cpu::rst_m0<0x10>, &Cpu::rst_m1<0x10>, &Cpu::rst_m2<0x10>, &Cpu::rst_m3<0x10> },
        /* D8 */ { &Cpu::ret_c_uu_m0<Flag::C>, &Cpu::ret_c_uu_m1<Flag::C>, &Cpu::ret_c_uu_m2<Flag::C>,
                    &Cpu::ret_c_uu_m3<Flag::C>, &Cpu::ret_c_uu_m4<Flag::C> },
        /* D9 */ { &Cpu::reti_uu_m0, &Cpu::reti_uu_m1, &Cpu::reti_uu_m2, &Cpu::reti_uu_m3 },
        /* DA */ { &Cpu::jp_c_uu_m0<Flag::C>, &Cpu::jp_c_uu_m1<Flag::C>, &Cpu::jp_c_uu_m2<Flag::C>, &Cpu::jp_c_uu_m3<Flag::C> },
        /* DB */ { &Cpu::invalid_microop },
        /* DC */ { &Cpu::call_c_uu_m0<Flag::C>, &Cpu::call_c_uu_m1<Flag::C>, &Cpu::call_c_uu_m2<Flag::C>,
                    &Cpu::call_c_uu_m3<Flag::C>, &Cpu::call_c_uu_m4<Flag::C>, &Cpu::call_c_uu_m5<Flag::C>, },
        /* DD */ { &Cpu::invalid_microop },
        /* DE */ { &Cpu::sbc_u_m0, &Cpu::sbc_u_m1 },
        /* DF */ { &Cpu::rst_m0<0x18>, &Cpu::rst_m1<0x18>, &Cpu::rst_m2<0x18>, &Cpu::rst_m3<0x18> },
        /* E0 */ { &Cpu::ldh_an_r_m0<Register8::A>, &Cpu::ldh_an_r_m1<Register8::A>, &Cpu::ldh_an_r_m2<Register8::A> },
        /* E1 */ { &Cpu::pop_rr_m0<Register16::HL>, &Cpu::pop_rr_m1<Register16::HL>, &Cpu::pop_rr_m2<Register16::HL> },
        /* E2 */ { &Cpu::ldh_ar_r_m0<Register8::C, Register8::A>, &Cpu::ldh_ar_r_m1<Register8::C, Register8::A> },
        /* E3 */ { &Cpu::invalid_microop },
        /* E4 */ { &Cpu::invalid_microop },
        /* E5 */ { &Cpu::push_rr_m0<Register16::HL>, &Cpu::push_rr_m1<Register16::HL>, &Cpu::push_rr_m2<Register16::HL>, &Cpu::push_rr_m3<Register16::HL> },
        /* E6 */ { &Cpu::and_u_m0, &Cpu::and_u_m1 },
        /* E7 */ { &Cpu::rst_m0<0x20>, &Cpu::rst_m1<0x20>, &Cpu::rst_m2<0x20>, &Cpu::rst_m3<0x20> },
        /* E8 */ { &Cpu::add_rr_s_m0<Register16::SP>, &Cpu::add_rr_s_m1<Register16::SP>, &Cpu::add_rr_s_m2<Register16::SP>, &Cpu::add_rr_s_m3<Register16::SP> },
        /* E9 */ { &Cpu::jp_rr_m0<Register16::HL> },
        /* EA */ { &Cpu::ld_ann_r_m0<Register8::A>, &Cpu::ld_ann_r_m1<Register8::A>, &Cpu::ld_ann_r_m2<Register8::A>, &Cpu::ld_ann_r_m3<Register8::A> },
        /* EB */ { &Cpu::invalid_microop },
        /* EC */ { &Cpu::invalid_microop },
        /* ED */ { &Cpu::invalid_microop },
        /* EE */ { &Cpu::xor_u_m0, &Cpu::xor_u_m1 },
        /* EF */ { &Cpu::rst_m0<0x28>, &Cpu::rst_m1<0x28>, &Cpu::rst_m2<0x28>, &Cpu::rst_m3<0x28> },
        /* F0 */ { &Cpu::ldh_r_an_m0<Register8::A>, &Cpu::ldh_r_an_m1<Register8::A>, &Cpu::ldh_r_an_m2<Register8::A> },
        /* F1 */ { &Cpu::pop_rr_m0<Register16::AF>, &Cpu::pop_rr_m1<Register16::AF>, &Cpu::pop_rr_m2<Register16::AF> },
        /* F2 */ { &Cpu::ldh_r_ar_m0<Register8::A, Register8::C>, &Cpu::ldh_r_ar_m1<Register8::A, Register8::C> },
        /* F3 */ { &Cpu::di_m0 },
        /* F4 */ { &Cpu::invalid_microop },
        /* F5 */ { &Cpu::push_rr_m0<Register16::AF>, &Cpu::push_rr_m1<Register16::AF>, &Cpu::push_rr_m2<Register16::AF>, &Cpu::push_rr_m3<Register16::AF> },
        /* F6 */ { &Cpu::or_u_m0, &Cpu::or_u_m1 },
        /* F7 */ { &Cpu::rst_m0<0x30>, &Cpu::rst_m1<0x30>, &Cpu::rst_m2<0x30>, &Cpu::rst_m3<0x30> },
        /* F8 */ { &Cpu::ld_rr_rrs_m0<Register16::HL, Register16::SP>, &Cpu::ld_rr_rrs_m1<Register16::HL, Register16::SP>, &Cpu::ld_rr_rrs_m2<Register16::HL, Register16::SP> },
        /* F9 */ { &Cpu::ld_rr_rr_m0<Register16::SP, Register16::HL>, &Cpu::ld_rr_rr_m1<Register16::SP, Register16::HL> },
        /* FA */ { &Cpu::ld_r_ann_m0<Register8::A>, &Cpu::ld_r_ann_m1<Register8::A>, &Cpu::ld_r_ann_m2<Register8::A>, &Cpu::ld_r_ann_m3<Register8::A> },
        /* FB */ { &Cpu::ei_m0 },
        /* FC */ { &Cpu::invalid_microop },
        /* FD */ { &Cpu::invalid_microop },
        /* FE */ { &Cpu::cp_u_m0, &Cpu::cp_u_m1 },
        /* FF */ { &Cpu::rst_m0<0x38>, &Cpu::rst_m1<0x38>, &Cpu::rst_m2<0x38>, &Cpu::rst_m3<0x38> },
    },
    instructions_cb {
        /* 00 */ { &Cpu::rlc_r_m0<Register8::B> },
        /* 01 */ { &Cpu::rlc_r_m0<Register8::C> },
        /* 02 */ { &Cpu::rlc_r_m0<Register8::D> },
        /* 03 */ { &Cpu::rlc_r_m0<Register8::E> },
        /* 04 */ { &Cpu::rlc_r_m0<Register8::H> },
        /* 05 */ { &Cpu::rlc_r_m0<Register8::L> },
        /* 06 */ { &Cpu::rlc_arr_m0<Register16::HL>,    &Cpu::rlc_arr_m1<Register16::HL>,    &Cpu::rlc_arr_m2<Register16::HL> },
        /* 07 */ { &Cpu::rlc_r_m0<Register8::A> },
        /* 08 */ { &Cpu::rrc_r_m0<Register8::B> },
        /* 09 */ { &Cpu::rrc_r_m0<Register8::C> },
        /* 0A */ { &Cpu::rrc_r_m0<Register8::D> },
        /* 0B */ { &Cpu::rrc_r_m0<Register8::E> },
        /* 0C */ { &Cpu::rrc_r_m0<Register8::H> },
        /* 0D */ { &Cpu::rrc_r_m0<Register8::L> },
        /* 0E */ { &Cpu::rrc_arr_m0<Register16::HL>,    &Cpu::rrc_arr_m1<Register16::HL>,    &Cpu::rrc_arr_m2<Register16::HL> },
        /* 0F */ { &Cpu::rrc_r_m0<Register8::A> },
        /* 10 */ { &Cpu::rl_r_m0<Register8::B> },
        /* 11 */ { &Cpu::rl_r_m0<Register8::C> },
        /* 12 */ { &Cpu::rl_r_m0<Register8::D> },
        /* 13 */ { &Cpu::rl_r_m0<Register8::E> },
        /* 14 */ { &Cpu::rl_r_m0<Register8::H> },
        /* 15 */ { &Cpu::rl_r_m0<Register8::L> },
        /* 16 */ { &Cpu::rl_arr_m0<Register16::HL>,     &Cpu::rl_arr_m1<Register16::HL>,     &Cpu::rl_arr_m2<Register16::HL> },
        /* 17 */ { &Cpu::rl_r_m0<Register8::A> },
        /* 18 */ { &Cpu::rr_r_m0<Register8::B> },
        /* 19 */ { &Cpu::rr_r_m0<Register8::C> },
        /* 1A */ { &Cpu::rr_r_m0<Register8::D> },
        /* 1B */ { &Cpu::rr_r_m0<Register8::E> },
        /* 1C */ { &Cpu::rr_r_m0<Register8::H> },
        /* 1D */ { &Cpu::rr_r_m0<Register8::L> },
        /* 1E */ { &Cpu::rr_arr_m0<Register16::HL>,     &Cpu::rr_arr_m1<Register16::HL>,     &Cpu::rr_arr_m2<Register16::HL> },
        /* 1F */ { &Cpu::rr_r_m0<Register8::A> },
        /* 20 */ { &Cpu::sla_r_m0<Register8::B> },
        /* 21 */ { &Cpu::sla_r_m0<Register8::C> },
        /* 22 */ { &Cpu::sla_r_m0<Register8::D> },
        /* 23 */ { &Cpu::sla_r_m0<Register8::E> },
        /* 24 */ { &Cpu::sla_r_m0<Register8::H> },
        /* 25 */ { &Cpu::sla_r_m0<Register8::L> },
        /* 26 */ { &Cpu::sla_arr_m0<Register16::HL>,    &Cpu::sla_arr_m1<Register16::HL>,    &Cpu::sla_arr_m2<Register16::HL>  },
        /* 27 */ { &Cpu::sla_r_m0<Register8::A> },
        /* 28 */ { &Cpu::sra_r_m0<Register8::B> },
        /* 29 */ { &Cpu::sra_r_m0<Register8::C> },
        /* 2A */ { &Cpu::sra_r_m0<Register8::D> },
        /* 2B */ { &Cpu::sra_r_m0<Register8::E> },
        /* 2C */ { &Cpu::sra_r_m0<Register8::H> },
        /* 2D */ { &Cpu::sra_r_m0<Register8::L> },
        /* 2E */ { &Cpu::sra_arr_m0<Register16::HL>,    &Cpu::sra_arr_m1<Register16::HL>,    &Cpu::sra_arr_m2<Register16::HL> },
        /* 2F */ { &Cpu::sra_r_m0<Register8::A> },
        /* 30 */ { &Cpu::swap_r_m0<Register8::B> },
        /* 31 */ { &Cpu::swap_r_m0<Register8::C> },
        /* 32 */ { &Cpu::swap_r_m0<Register8::D> },
        /* 33 */ { &Cpu::swap_r_m0<Register8::E> },
        /* 34 */ { &Cpu::swap_r_m0<Register8::H> },
        /* 35 */ { &Cpu::swap_r_m0<Register8::L> },
        /* 36 */ { &Cpu::swap_arr_m0<Register16::HL>,   &Cpu::swap_arr_m1<Register16::HL>,   &Cpu::swap_arr_m2<Register16::HL> },
        /* 37 */ { &Cpu::swap_r_m0<Register8::A> },
        /* 38 */ { &Cpu::srl_r_m0<Register8::B> },
        /* 39 */ { &Cpu::srl_r_m0<Register8::C> },
        /* 3A */ { &Cpu::srl_r_m0<Register8::D> },
        /* 3B */ { &Cpu::srl_r_m0<Register8::E> },
        /* 3C */ { &Cpu::srl_r_m0<Register8::H> },
        /* 3D */ { &Cpu::srl_r_m0<Register8::L> },
        /* 3E */ { &Cpu::srl_arr_m0<Register16::HL>,    &Cpu::srl_arr_m1<Register16::HL>,    &Cpu::srl_arr_m2<Register16::HL> },
        /* 3F */ { &Cpu::srl_r_m0<Register8::A> },
        /* 40 */ { &Cpu::bit_r_m0<0, Register8::B> },
        /* 41 */ { &Cpu::bit_r_m0<0, Register8::C> },
        /* 42 */ { &Cpu::bit_r_m0<0, Register8::D> },
        /* 43 */ { &Cpu::bit_r_m0<0, Register8::E> },
        /* 44 */ { &Cpu::bit_r_m0<0, Register8::H> },
        /* 45 */ { &Cpu::bit_r_m0<0, Register8::L> },
        /* 46 */ { &Cpu::bit_arr_m0<0, Register16::HL>, &Cpu::bit_arr_m1<0, Register16::HL> },
        /* 47 */ { &Cpu::bit_r_m0<0, Register8::A> },
        /* 48 */ { &Cpu::bit_r_m0<1, Register8::B> },
        /* 49 */ { &Cpu::bit_r_m0<1, Register8::C> },
        /* 4A */ { &Cpu::bit_r_m0<1, Register8::D> },
        /* 4B */ { &Cpu::bit_r_m0<1, Register8::E> },
        /* 4C */ { &Cpu::bit_r_m0<1, Register8::H> },
        /* 4D */ { &Cpu::bit_r_m0<1, Register8::L> },
        /* 4E */ { &Cpu::bit_arr_m0<1, Register16::HL>, &Cpu::bit_arr_m1<1, Register16::HL> },
        /* 4F */ { &Cpu::bit_r_m0<1, Register8::A> },
        /* 50 */ { &Cpu::bit_r_m0<2, Register8::B> },
        /* 51 */ { &Cpu::bit_r_m0<2, Register8::C> },
        /* 52 */ { &Cpu::bit_r_m0<2, Register8::D> },
        /* 53 */ { &Cpu::bit_r_m0<2, Register8::E> },
        /* 54 */ { &Cpu::bit_r_m0<2, Register8::H> },
        /* 55 */ { &Cpu::bit_r_m0<2, Register8::L> },
        /* 56 */ { &Cpu::bit_arr_m0<2, Register16::HL>, &Cpu::bit_arr_m1<2, Register16::HL> },
        /* 57 */ { &Cpu::bit_r_m0<2, Register8::A> },
        /* 58 */ { &Cpu::bit_r_m0<3, Register8::B> },
        /* 59 */ { &Cpu::bit_r_m0<3, Register8::C> },
        /* 5A */ { &Cpu::bit_r_m0<3, Register8::D> },
        /* 5B */ { &Cpu::bit_r_m0<3, Register8::E> },
        /* 5C */ { &Cpu::bit_r_m0<3, Register8::H> },
        /* 5D */ { &Cpu::bit_r_m0<3, Register8::L> },
        /* 5E */ { &Cpu::bit_arr_m0<3, Register16::HL>, &Cpu::bit_arr_m1<3, Register16::HL> },
        /* 5F */ { &Cpu::bit_r_m0<3, Register8::A> },
        /* 60 */ { &Cpu::bit_r_m0<4, Register8::B> },
        /* 61 */ { &Cpu::bit_r_m0<4, Register8::C> },
        /* 62 */ { &Cpu::bit_r_m0<4, Register8::D> },
        /* 63 */ { &Cpu::bit_r_m0<4, Register8::E> },
        /* 64 */ { &Cpu::bit_r_m0<4, Register8::H> },
        /* 65 */ { &Cpu::bit_r_m0<4, Register8::L> },
        /* 66 */ { &Cpu::bit_arr_m0<4, Register16::HL>, &Cpu::bit_arr_m1<4, Register16::HL> },
        /* 67 */ { &Cpu::bit_r_m0<4, Register8::A> },
        /* 68 */ { &Cpu::bit_r_m0<5, Register8::B> },
        /* 69 */ { &Cpu::bit_r_m0<5, Register8::C> },
        /* 6A */ { &Cpu::bit_r_m0<5, Register8::D> },
        /* 6B */ { &Cpu::bit_r_m0<5, Register8::E> },
        /* 6C */ { &Cpu::bit_r_m0<5, Register8::H> },
        /* 6D */ { &Cpu::bit_r_m0<5, Register8::L> },
        /* 6E */ { &Cpu::bit_arr_m0<5, Register16::HL>, &Cpu::bit_arr_m1<5, Register16::HL> },
        /* 6F */ { &Cpu::bit_r_m0<5, Register8::A> },
        /* 70 */ { &Cpu::bit_r_m0<6, Register8::B> },
        /* 71 */ { &Cpu::bit_r_m0<6, Register8::C> },
        /* 72 */ { &Cpu::bit_r_m0<6, Register8::D> },
        /* 73 */ { &Cpu::bit_r_m0<6, Register8::E> },
        /* 74 */ { &Cpu::bit_r_m0<6, Register8::H> },
        /* 75 */ { &Cpu::bit_r_m0<6, Register8::L> },
        /* 76 */ { &Cpu::bit_arr_m0<6, Register16::HL>, &Cpu::bit_arr_m1<6, Register16::HL> },
        /* 77 */ { &Cpu::bit_r_m0<6, Register8::A> },
        /* 78 */ { &Cpu::bit_r_m0<7, Register8::B> },
        /* 79 */ { &Cpu::bit_r_m0<7, Register8::C> },
        /* 7A */ { &Cpu::bit_r_m0<7, Register8::D> },
        /* 7B */ { &Cpu::bit_r_m0<7, Register8::E> },
        /* 7C */ { &Cpu::bit_r_m0<7, Register8::H> },
        /* 7D */ { &Cpu::bit_r_m0<7, Register8::L> },
        /* 7E */ { &Cpu::bit_arr_m0<7, Register16::HL>, &Cpu::bit_arr_m1<7, Register16::HL> },
        /* 7F */ { &Cpu::bit_r_m0<7, Register8::A> },
        /* 80 */ { &Cpu::res_r_m0<0, Register8::B> },
        /* 81 */ { &Cpu::res_r_m0<0, Register8::C> },
        /* 82 */ { &Cpu::res_r_m0<0, Register8::D> },
        /* 83 */ { &Cpu::res_r_m0<0, Register8::E> },
        /* 84 */ { &Cpu::res_r_m0<0, Register8::H> },
        /* 85 */ { &Cpu::res_r_m0<0, Register8::L> },
        /* 86 */ { &Cpu::res_arr_m0<0, Register16::HL>, &Cpu::res_arr_m1<0, Register16::HL>, &Cpu::res_arr_m2<0, Register16::HL> },
        /* 87 */ { &Cpu::res_r_m0<0, Register8::A> },
        /* 88 */ { &Cpu::res_r_m0<1, Register8::B> },
        /* 89 */ { &Cpu::res_r_m0<1, Register8::C> },
        /* 8A */ { &Cpu::res_r_m0<1, Register8::D> },
        /* 8B */ { &Cpu::res_r_m0<1, Register8::E> },
        /* 8C */ { &Cpu::res_r_m0<1, Register8::H> },
        /* 8D */ { &Cpu::res_r_m0<1, Register8::L> },
        /* 8E */ { &Cpu::res_arr_m0<1, Register16::HL>, &Cpu::res_arr_m1<1, Register16::HL>, &Cpu::res_arr_m2<1, Register16::HL> },
        /* 8F */ { &Cpu::res_r_m0<1, Register8::A> },
        /* 90 */ { &Cpu::res_r_m0<2, Register8::B> },
        /* 91 */ { &Cpu::res_r_m0<2, Register8::C> },
        /* 92 */ { &Cpu::res_r_m0<2, Register8::D> },
        /* 93 */ { &Cpu::res_r_m0<2, Register8::E> },
        /* 94 */ { &Cpu::res_r_m0<2, Register8::H> },
        /* 95 */ { &Cpu::res_r_m0<2, Register8::L> },
        /* 96 */ { &Cpu::res_arr_m0<2, Register16::HL>, &Cpu::res_arr_m1<2, Register16::HL>, &Cpu::res_arr_m2<2, Register16::HL> },
        /* 97 */ { &Cpu::res_r_m0<2, Register8::A> },
        /* 98 */ { &Cpu::res_r_m0<3, Register8::B> },
        /* 99 */ { &Cpu::res_r_m0<3, Register8::C> },
        /* 9A */ { &Cpu::res_r_m0<3, Register8::D> },
        /* 9B */ { &Cpu::res_r_m0<3, Register8::E> },
        /* 9C */ { &Cpu::res_r_m0<3, Register8::H> },
        /* 9D */ { &Cpu::res_r_m0<3, Register8::L> },
        /* 9E */ { &Cpu::res_arr_m0<3, Register16::HL>, &Cpu::res_arr_m1<3, Register16::HL>, &Cpu::res_arr_m2<3, Register16::HL> },
        /* 9F */ { &Cpu::res_r_m0<3, Register8::A> },
        /* A0 */ { &Cpu::res_r_m0<4, Register8::B> },
        /* A1 */ { &Cpu::res_r_m0<4, Register8::C> },
        /* A2 */ { &Cpu::res_r_m0<4, Register8::D> },
        /* A3 */ { &Cpu::res_r_m0<4, Register8::E> },
        /* A4 */ { &Cpu::res_r_m0<4, Register8::H> },
        /* A5 */ { &Cpu::res_r_m0<4, Register8::L> },
        /* A6 */ { &Cpu::res_arr_m0<4, Register16::HL>, &Cpu::res_arr_m1<4, Register16::HL>, &Cpu::res_arr_m2<4, Register16::HL> },
        /* A7 */ { &Cpu::res_r_m0<4, Register8::A> },
        /* A8 */ { &Cpu::res_r_m0<5, Register8::B> },
        /* A9 */ { &Cpu::res_r_m0<5, Register8::C> },
        /* AA */ { &Cpu::res_r_m0<5, Register8::D> },
        /* AB */ { &Cpu::res_r_m0<5, Register8::E> },
        /* AC */ { &Cpu::res_r_m0<5, Register8::H> },
        /* AD */ { &Cpu::res_r_m0<5, Register8::L> },
        /* AE */ { &Cpu::res_arr_m0<5, Register16::HL>, &Cpu::res_arr_m1<5, Register16::HL>, &Cpu::res_arr_m2<5, Register16::HL> },
        /* AF */ { &Cpu::res_r_m0<5, Register8::A> },
        /* B0 */ { &Cpu::res_r_m0<6, Register8::B> },
        /* B1 */ { &Cpu::res_r_m0<6, Register8::C> },
        /* B2 */ { &Cpu::res_r_m0<6, Register8::D> },
        /* B3 */ { &Cpu::res_r_m0<6, Register8::E> },
        /* B4 */ { &Cpu::res_r_m0<6, Register8::H> },
        /* B5 */ { &Cpu::res_r_m0<6, Register8::L> },
        /* B6 */ { &Cpu::res_arr_m0<6, Register16::HL>, &Cpu::res_arr_m1<6, Register16::HL>, &Cpu::res_arr_m2<6, Register16::HL> },
        /* B7 */ { &Cpu::res_r_m0<6, Register8::A> },
        /* B8 */ { &Cpu::res_r_m0<7, Register8::B> },
        /* B9 */ { &Cpu::res_r_m0<7, Register8::C> },
        /* BA */ { &Cpu::res_r_m0<7, Register8::D> },
        /* BB */ { &Cpu::res_r_m0<7, Register8::E> },
        /* BC */ { &Cpu::res_r_m0<7, Register8::H> },
        /* BD */ { &Cpu::res_r_m0<7, Register8::L> },
        /* BE */ { &Cpu::res_arr_m0<7, Register16::HL>, &Cpu::res_arr_m1<7, Register16::HL>, &Cpu::res_arr_m2<7, Register16::HL> },
        /* BF */ { &Cpu::res_r_m0<7, Register8::A> },
        /* C0 */ { &Cpu::set_r_m0<0, Register8::B> },
        /* C1 */ { &Cpu::set_r_m0<0, Register8::C> },
        /* C2 */ { &Cpu::set_r_m0<0, Register8::D> },
        /* C3 */ { &Cpu::set_r_m0<0, Register8::E> },
        /* C4 */ { &Cpu::set_r_m0<0, Register8::H> },
        /* C5 */ { &Cpu::set_r_m0<0, Register8::L> },
        /* C6 */ { &Cpu::set_arr_m0<0, Register16::HL>, &Cpu::set_arr_m1<0, Register16::HL>, &Cpu::set_arr_m2<0, Register16::HL> },
        /* C7 */ { &Cpu::set_r_m0<0, Register8::A> },
        /* C8 */ { &Cpu::set_r_m0<1, Register8::B> },
        /* C9 */ { &Cpu::set_r_m0<1, Register8::C> },
        /* CA */ { &Cpu::set_r_m0<1, Register8::D> },
        /* CB */ { &Cpu::set_r_m0<1, Register8::E> },
        /* CC */ { &Cpu::set_r_m0<1, Register8::H> },
        /* CD */ { &Cpu::set_r_m0<1, Register8::L> },
        /* CE */ { &Cpu::set_arr_m0<1, Register16::HL>, &Cpu::set_arr_m1<1, Register16::HL>, &Cpu::set_arr_m2<1, Register16::HL> },
        /* CF */ { &Cpu::set_r_m0<1, Register8::A> },
        /* D0 */ { &Cpu::set_r_m0<2, Register8::B> },
        /* D1 */ { &Cpu::set_r_m0<2, Register8::C> },
        /* D2 */ { &Cpu::set_r_m0<2, Register8::D> },
        /* D3 */ { &Cpu::set_r_m0<2, Register8::E> },
        /* D4 */ { &Cpu::set_r_m0<2, Register8::H> },
        /* D5 */ { &Cpu::set_r_m0<2, Register8::L> },
        /* D6 */ { &Cpu::set_arr_m0<2, Register16::HL>, &Cpu::set_arr_m1<2, Register16::HL>, &Cpu::set_arr_m2<2, Register16::HL> },
        /* D7 */ { &Cpu::set_r_m0<2, Register8::A> },
        /* D8 */ { &Cpu::set_r_m0<3, Register8::B> },
        /* D9 */ { &Cpu::set_r_m0<3, Register8::C> },
        /* DA */ { &Cpu::set_r_m0<3, Register8::D> },
        /* DB */ { &Cpu::set_r_m0<3, Register8::E> },
        /* DC */ { &Cpu::set_r_m0<3, Register8::H> },
        /* DD */ { &Cpu::set_r_m0<3, Register8::L> },
        /* DE */ { &Cpu::set_arr_m0<3, Register16::HL>, &Cpu::set_arr_m1<3, Register16::HL>, &Cpu::set_arr_m2<3, Register16::HL> },
        /* DF */ { &Cpu::set_r_m0<3, Register8::A> },
        /* E0 */ { &Cpu::set_r_m0<4, Register8::B> },
        /* E1 */ { &Cpu::set_r_m0<4, Register8::C> },
        /* E2 */ { &Cpu::set_r_m0<4, Register8::D> },
        /* E3 */ { &Cpu::set_r_m0<4, Register8::E> },
        /* E4 */ { &Cpu::set_r_m0<4, Register8::H> },
        /* E5 */ { &Cpu::set_r_m0<4, Register8::L> },
        /* E6 */ { &Cpu::set_arr_m0<4, Register16::HL>, &Cpu::set_arr_m1<4, Register16::HL>, &Cpu::set_arr_m2<4, Register16::HL> },
        /* E7 */ { &Cpu::set_r_m0<4, Register8::A> },
        /* E8 */ { &Cpu::set_r_m0<5, Register8::B> },
        /* E9 */ { &Cpu::set_r_m0<5, Register8::C> },
        /* EA */ { &Cpu::set_r_m0<5, Register8::D> },
        /* EB */ { &Cpu::set_r_m0<5, Register8::E> },
        /* EC */ { &Cpu::set_r_m0<5, Register8::H> },
        /* ED */ { &Cpu::set_r_m0<5, Register8::L> },
        /* EE */ { &Cpu::set_arr_m0<5, Register16::HL>, &Cpu::set_arr_m1<5, Register16::HL>, &Cpu::set_arr_m2<5, Register16::HL> },
        /* EF */ { &Cpu::set_r_m0<5, Register8::A> },
        /* F0 */ { &Cpu::set_r_m0<6, Register8::B> },
        /* F1 */ { &Cpu::set_r_m0<6, Register8::C> },
        /* F2 */ { &Cpu::set_r_m0<6, Register8::D> },
        /* F3 */ { &Cpu::set_r_m0<6, Register8::E> },
        /* F4 */ { &Cpu::set_r_m0<6, Register8::H> },
        /* F5 */ { &Cpu::set_r_m0<6, Register8::L> },
        /* F6 */ { &Cpu::set_arr_m0<6, Register16::HL>, &Cpu::set_arr_m1<6, Register16::HL>, &Cpu::set_arr_m2<6, Register16::HL> },
        /* F7 */ { &Cpu::set_r_m0<6, Register8::A> },
        /* F8 */ { &Cpu::set_r_m0<7, Register8::B> },
        /* F9 */ { &Cpu::set_r_m0<7, Register8::C> },
        /* FA */ { &Cpu::set_r_m0<7, Register8::D> },
        /* FB */ { &Cpu::set_r_m0<7, Register8::E> },
        /* FC */ { &Cpu::set_r_m0<7, Register8::H> },
        /* FD */ { &Cpu::set_r_m0<7, Register8::L> },
        /* FE */ { &Cpu::set_arr_m0<7, Register16::HL>, &Cpu::set_arr_m1<7, Register16::HL>, &Cpu::set_arr_m2<7, Register16::HL> },
        /* FF */ { &Cpu::set_r_m0<7, Register8::A> },
    },
    isr { &Cpu::isr_m0, &Cpu::isr_m1, &Cpu::isr_m2, &Cpu::isr_m3, &Cpu::isr_m4 } // clang-format on
{
}

void Cpu::tick_t0() {
#ifdef ENABLE_ASSERTS
    assert_tick<3>();
#endif

    check_interrupt<3>();
    tick();
}

void Cpu::tick_t1() {
#ifdef ENABLE_ASSERTS
    assert_tick<0>();
#endif

    check_interrupt<0>();
    flush_write();
    idu.tick_t1();
}

void Cpu::tick_t2() {
#ifdef ENABLE_ASSERTS
    assert_tick<1>();
#endif

    check_interrupt<1>();
}

void Cpu::tick_t3() {
#ifdef ENABLE_ASSERTS
    assert_tick<2>();
#endif

    check_interrupt<2>();
    flush_read();

#ifdef ENABLE_DEBUGGER
    // Update instruction's debug information
    // TODO: verify this
    if (!halted) {
        if (fetching) {
            instruction.address = pc - 1;
            instruction.cycle_microop = 0;
        } else {
            instruction.cycle_microop++;
        }
    }
#endif

#ifdef ENABLE_TESTS
    if (fetching) {
        instruction.opcode = io.data;
    }
#endif
}

inline void Cpu::tick() {
    // Eventually handle pending interrupt
    if (interrupt.state == InterruptState::Pending) {
        // Decrease the remaining ticks for the pending interrupt
        if (interrupt.remaining_ticks > 0) {
            --interrupt.remaining_ticks;
        }

        // Handle it if the countdown is finished (but only at the beginning of a new instruction)
        if (interrupt.remaining_ticks == 0 && instruction.microop.counter == 0) {
            halted = false;

            // Serve the interrupt if IME is enabled
            if (ime == ImeState::Enabled) {
                interrupt.state = InterruptState::Serving;
                serve_interrupt();
            } else {
                interrupt.state = InterruptState::None;
            }
        }
    }

    if (halted) {
#ifdef ENABLE_DEBUGGER
        ++cycles;
#endif
        return;
    }

    // Eventually fetch a new instruction
    if (fetching) {
        fetching = false;
        instruction.microop.selector = &instructions[io.data][0];
    } else if (fetching_cb) {
        fetching_cb = false;
        instruction.microop.selector = &instructions_cb[io.data][0];
    }

    const MicroOperation microop = *instruction.microop.selector;

    // Advance to next micro operation
    ++instruction.microop.counter;
    ++instruction.microop.selector;

    // Execute current micro operation
    (this->*microop)();

    // Eventually advance IME state (only at the end of an instruction).
    if (instruction.microop.counter == 0 && ime > ImeState::Disabled && ime < ImeState::Enabled) {
        ime = static_cast<ImeState>(static_cast<uint8_t>(ime) + 1);
    }

#ifdef ENABLE_DEBUGGER
    ++cycles;
#endif
}

void Cpu::save_state(Parcel& parcel) const {
    parcel.write_bool(fetching);
    parcel.write_bool(halted);

    parcel.write_uint16(af);
    parcel.write_uint16(bc);
    parcel.write_uint16(de);
    parcel.write_uint16(hl);
    parcel.write_uint16(pc);
    parcel.write_uint16(sp);

    parcel.write_uint8((uint8_t)ime);

    parcel.write_uint8((uint8_t)interrupt.state);
    parcel.write_uint8(interrupt.remaining_ticks);

    parcel.write_bool(fetching_cb);

    if (static_cast<size_t>(instruction.microop.selector - &instructions[0][0]) <
        array_size(instructions) * INSTR_LEN) {
        parcel.write_uint8(STATE_INSTRUCTION_FLAG_NORMAL);
        parcel.write_uint16(instruction.microop.selector - &instructions[0][0]);
    } else if (static_cast<size_t>(instruction.microop.selector - &instructions_cb[0][0]) <
               array_size(instructions_cb) * INSTR_LEN) {
        parcel.write_uint8(STATE_INSTRUCTION_FLAG_CB);
        parcel.write_uint16(instruction.microop.selector - &instructions_cb[0][0]);
    } else if (static_cast<size_t>(instruction.microop.selector - &isr[0]) < INSTR_LEN) {
        parcel.write_uint8(STATE_INSTRUCTION_FLAG_ISR);
        parcel.write_uint16(instruction.microop.selector - &isr[0]);
    } else {
        ASSERT_NO_ENTRY();
    }

    parcel.write_uint8(instruction.microop.counter);

#ifdef ENABLE_DEBUGGER
    parcel.write_uint16(instruction.address);
    parcel.write_uint8(instruction.cycle_microop);
#endif

    parcel.write_uint8((uint8_t)io.state);
    parcel.write_uint8(io.data);
    parcel.write_bool(b);
    parcel.write_uint8(u);
    parcel.write_uint8(u2);
    parcel.write_uint8(lsb);
    parcel.write_uint8(msb);
    parcel.write_uint16(uu);
    parcel.write_uint16(addr);

#ifdef ENABLE_DEBUGGER
    parcel.write_uint64(cycles);
#endif
}

void Cpu::load_state(Parcel& parcel) {
    fetching = parcel.read_bool();
    halted = parcel.read_bool();

    af = parcel.read_int16();
    bc = parcel.read_int16();
    de = parcel.read_int16();
    hl = parcel.read_int16();
    pc = parcel.read_int16();
    sp = parcel.read_int16();

    ime = (ImeState)(parcel.read_uint8());

    interrupt.state = (InterruptState)parcel.read_uint8();
    interrupt.remaining_ticks = parcel.read_uint8();

    fetching_cb = parcel.read_bool();

    const uint8_t instruction_flag = parcel.read_uint8();
    const uint16_t offset = parcel.read_int16();

    if (instruction_flag == STATE_INSTRUCTION_FLAG_NORMAL) {
        instruction.microop.selector = &instructions[offset / INSTR_LEN][offset % INSTR_LEN];
    } else if (instruction_flag == STATE_INSTRUCTION_FLAG_CB) {
        instruction.microop.selector = &instructions_cb[offset / INSTR_LEN][offset % INSTR_LEN];
    } else if (instruction_flag == STATE_INSTRUCTION_FLAG_ISR) {
        instruction.microop.selector = &isr[offset];
    } else {
        ASSERT_NO_ENTRY();
    }

    instruction.microop.counter = parcel.read_uint8();

#ifdef ENABLE_DEBUGGER
    instruction.address = parcel.read_uint16();
    instruction.cycle_microop = parcel.read_uint8();
#endif

    io.state = (IO::State)parcel.read_uint8();
    io.data = parcel.read_uint8();
    b = parcel.read_bool();
    u = parcel.read_uint8();
    u2 = parcel.read_uint8();
    lsb = parcel.read_uint8();
    msb = parcel.read_uint8();
    uu = parcel.read_uint16();
    addr = parcel.read_uint16();

#ifdef ENABLE_DEBUGGER
    cycles = parcel.read_uint64();
#endif
}

void Cpu::reset() {
    fetching = false;
    halted = false;

#ifdef ENABLE_CGB
    af = if_bootrom_else(0, 0x1180);
    bc = 0x0000;
    de = if_bootrom_else(0, 0xFF56);
    hl = if_bootrom_else(0, 0x000D);
    pc = if_bootrom_else(0, 0x0100);
    sp = if_bootrom_else(0, 0xFFFE);
#else
    af = if_bootrom_else(0, 0x01B0);
    bc = if_bootrom_else(0, 0x0013);
    de = if_bootrom_else(0, 0x00D8);
    hl = if_bootrom_else(0, 0x014D);
    pc = if_bootrom_else(0, 0x0100);
    sp = if_bootrom_else(0, 0xFFFE);
#endif

    ime = ImeState::Disabled;
    interrupt.state = InterruptState::None;
    interrupt.remaining_ticks = 0;

    fetching_cb = false;

    instruction.microop.selector = nop;
    instruction.microop.counter = 0;

#ifdef ENABLE_TESTS
    instruction.opcode = 0;
#endif

#ifdef ENABLE_DEBUGGER
    instruction.address = 0;
    instruction.cycle_microop = 0;
#endif

    io.state = IO::State::Idle;
    io.data = 0;

    b = false;
    u = 0;
    u2 = 0;
    lsb = 0;
    msb = 0;
    uu = 0;
    addr = 0;

#ifdef ENABLE_DEBUGGER
    cycles = 0;
#endif
}

template <uint8_t t>
void Cpu::check_interrupt() {
    /*
     * Interrupt timings (blank spaces are unknown).
     *
     * -----------------------------------
     * NotHalted | T0  |  T1 |  T2 |  T3 |
     * -----------------------------------
     * VBlank    |  1  |  1  |     |     |
     * Stat      |  1  |  1  |  2  |  2  |
     * Timer     |  1  |  1  |     |  2  |
     * Serial    |  1  |  1  |     |  2  |
     * Joypad    |  1  |  1  |     |     |
     *
     * -----------------------------------
     * Halted    | T0  |  T1 |  T2 |  T3 |
     * -----------------------------------
     * VBlank    |  1  |     |     |     |
     * Stat      |  2  |  2  |  2  |  2  |
     * Timer     |     |     |     |  3  |
     * Serial    |     |     |     |  3  |
     * Joypad    |     |     |     |     |
     */

#ifdef ENABLE_ASSERTS
    static constexpr uint8_t UNKNOWN_INTERRUPT_TIMING = UINT8_MAX;
    static constexpr uint8_t U = UNKNOWN_INTERRUPT_TIMING;
#else
    static constexpr uint8_t U = 1;
#endif
    static constexpr uint8_t J = 1; // not sure about joypad timing

    // clang-format off
    static constexpr uint8_t INTERRUPTS_TIMINGS[32][2][4] /* [interrupt flags][halted][t-cycle] */ = {
        /*  0 : None */ {{U, U, U, U}, {U, U, U, U}},
        /*  1 : VBlank */ {{1, 1, U, U}, {1, U, U, U}},
        /*  2 : STAT */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /*  3 : STAT + VBlank */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /*  4 : Timer */ {{1, 1, U, 2}, {U, U, U, 3}},
        /*  5 : Timer + VBlank */ {{1, 1, U, 2}, {1, U, U, 3}},
        /*  6 : Timer + STAT */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /*  7 : Timer + STAT + VBlank */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /*  8 : Serial */ {{1, 1, U, 2}, {U, U, U, 3}},
        /*  9 : Serial + VBlank */ {{1, 1, U, 2}, {1, U, U, 3}},
        /* 10 : Serial + STAT */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /* 11 : Serial + STAT + VBlank */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /* 12 : Serial + Timer */ {{1, 1, U, 2}, {U, U, U, 3}},
        /* 13 : Serial + Timer + VBlank */ {{1, 1, U, 2}, {1, U, U, 3}},
        /* 14 : Serial + Timer + STAT */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /* 15 : Serial + Timer + STAT + VBlank */ {{1, 1, 1, 2}, {1, 2, 2, 2}},
        /* 16 : Joypad */ {{J, J, J, J}, {J, J, J, J}},
        /* 17 : Joypad + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 18 : Joypad + STAT */ {{J, J, J, J}, {J, J, J, J}},
        /* 19 : Joypad + STAT + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 20 : Joypad + Timer */ {{J, J, J, J}, {J, J, J, J}},
        /* 21 : Joypad + Timer + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 22 : Joypad + Timer + STAT */ {{J, J, J, J}, {J, J, J, J}},
        /* 23 : Joypad + Timer + STAT + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 24 : Joypad + Serial */ {{J, J, J, J}, {J, J, J, J}},
        /* 25 : Joypad + Serial + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 26 : Joypad + Serial + STAT */ {{J, J, J, J}, {J, J, J, J}},
        /* 27 : Joypad + Serial + STAT + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 28 : Joypad + Serial + Timer */ {{J, J, J, J}, {J, J, J, J}},
        /* 29 : Joypad + Serial + Timer + VBlank */ {{J, J, J, J}, {J, J, J, J}},
        /* 30 : Joypad + Serial + Timer + STAT */ {{J, J, J, J}, {J, J, J, J}},
        /* 31 : Joypad + Serial + Timer + STAT + VBlank */ {{J, J, J, J}, {J, J, J, J}},
    };
    // clang-format on

    if (interrupt.state == InterruptState::None && (halted || ime == ImeState::Enabled)) {
        if (const uint8_t pending_interrupts = get_pending_interrupts()) {
            interrupt.state = InterruptState::Pending;
            interrupt.remaining_ticks = INTERRUPTS_TIMINGS[pending_interrupts][halted][t];
            ASSERT_CODE({
                if (interrupt.remaining_ticks == UNKNOWN_INTERRUPT_TIMING) {
                    std::cerr << "---- UNKNOWN INTERRUPT TIMING ----" << std::endl;
                    std::cerr << "PC     : " << hex(pc) << std::endl;
                    std::cerr << "Halted : " << halted << std::endl;
                    std::cerr << "T      : " << +t << std::endl;
                    std::cerr << "IF     : " << bin((uint8_t)interrupts.IF) << std::endl;
                    std::cerr << "IE     : " << bin((uint8_t)interrupts.IE) << std::endl;
                    std::cerr << "VBlank : " << test_bit<Specs::Bits::Interrupts::VBLANK>(interrupts.IF & interrupts.IE)
                              << std::endl;
                    std::cerr << "STAT   : " << test_bit<Specs::Bits::Interrupts::STAT>(interrupts.IF & interrupts.IE)
                              << std::endl;
                    std::cerr << "TIMER  : " << test_bit<Specs::Bits::Interrupts::TIMER>(interrupts.IF & interrupts.IE)
                              << std::endl;
                    std::cerr << "SERIAL : " << test_bit<Specs::Bits::Interrupts::SERIAL>(interrupts.IF & interrupts.IE)
                              << std::endl;
                    std::cerr << "JOYPAD : " << test_bit<Specs::Bits::Interrupts::JOYPAD>(interrupts.IF & interrupts.IE)
                              << std::endl;
                    FATAL("Unknown interrupt timing: this should be fixed by the developer");
                }
            });
        }
    }
}

inline uint8_t Cpu::get_pending_interrupts() const {
    return interrupts.IE & interrupts.IF & bitmask<5>;
}

void Cpu::serve_interrupt() {
    ASSERT(fetching);
    ASSERT(interrupt.state == InterruptState::Serving);
    fetching = false;
    instruction.microop.counter = 0;
    instruction.microop.selector = &isr[0];
}

inline void Cpu::fetch() {
    instruction.microop.counter = 0;
    fetching = true;
    read(pc);
    idu.increment(pc);
}

inline void Cpu::fetch_cb() {
    ASSERT(instruction.microop.counter == 1);
    fetching_cb = true;
    read(pc);
    idu.increment(pc);
}

inline void Cpu::read(uint16_t address) {
    mmu.read_request(address);
    io.state = IO::State::Read;
}

inline void Cpu::write(uint16_t address, uint8_t value) {
    mmu.write_request(address);
    io.state = IO::State::Write;
    io.data = value;
}

inline void Cpu::flush_read() {
    if (io.state == IO::State::Read) {
        io.data = mmu.flush_read_request();
        io.state = IO::State::Idle;
    }
}

inline void Cpu::flush_write() {
    if (io.state == IO::State::Write) {
        mmu.flush_write_request(io.data);
        io.state = IO::State::Idle;
    }
}

template <Cpu::Register8 r>
uint8_t Cpu::read_reg8() const {
    if constexpr (r == Register8::A) {
        return get_byte<1>(af);
    }
    if constexpr (r == Register8::B) {
        return get_byte<1>(bc);
    }
    if constexpr (r == Register8::C) {
        return get_byte<0>(bc);
    }
    if constexpr (r == Register8::D) {
        return get_byte<1>(de);
    }
    if constexpr (r == Register8::E) {
        return get_byte<0>(de);
    }
    if constexpr (r == Register8::F) {
        // Last four bits hardwired to 0
        return get_byte<0>(af) & 0xF0;
    }
    if constexpr (r == Register8::H) {
        return get_byte<1>(hl);
    }
    if constexpr (r == Register8::L) {
        return get_byte<0>(hl);
    }
    if constexpr (r == Register8::SP_H) {
        return get_byte<1>(sp);
    }
    if constexpr (r == Register8::SP_L) {
        return get_byte<0>(sp);
    }
    if constexpr (r == Register8::PC_H) {
        return get_byte<1>(pc);
    }
    if constexpr (r == Register8::PC_L) {
        return get_byte<0>(pc);
    }
}

template <Cpu::Register8 r>
void Cpu::write_reg8(uint8_t value) {
    if constexpr (r == Register8::A) {
        set_byte<1>(af, value);
    } else if constexpr (r == Register8::B) {
        set_byte<1>(bc, value);
    } else if constexpr (r == Register8::C) {
        set_byte<0>(bc, value);
    } else if constexpr (r == Register8::D) {
        set_byte<1>(de, value);
    } else if constexpr (r == Register8::E) {
        set_byte<0>(de, value);
    } else if constexpr (r == Register8::F) {
        set_byte<0>(af, value & 0xF0);
    } else if constexpr (r == Register8::H) {
        set_byte<1>(hl, value);
    } else if constexpr (r == Register8::L) {
        set_byte<0>(hl, value);
    } else if constexpr (r == Register8::SP_H) {
        set_byte<1>(sp, value);
    } else if constexpr (r == Register8::SP_L) {
        set_byte<0>(sp, value);
    } else if constexpr (r == Register8::PC_H) {
        set_byte<1>(pc, value);
    } else if constexpr (r == Register8::PC_L) {
        set_byte<0>(pc, value);
    }
}

template <Cpu::Register16 rr>
uint16_t Cpu::read_reg16() const {
    if constexpr (rr == Register16::AF) {
        return af & 0xFFF0;
    }
    if constexpr (rr == Register16::BC) {
        return bc;
    }
    if constexpr (rr == Register16::DE) {
        return de;
    }
    if constexpr (rr == Register16::HL) {
        return hl;
    }
    if constexpr (rr == Register16::PC) {
        return pc;
    }
    if constexpr (rr == Register16::SP) {
        return sp;
    }
}

template <Cpu::Register16 rr>
void Cpu::write_reg16(uint16_t value) {
    if constexpr (rr == Register16::AF) {
        af = value & 0xFFF0;
    } else if constexpr (rr == Register16::BC) {
        bc = value;
    } else if constexpr (rr == Register16::DE) {
        de = value;
    } else if constexpr (rr == Register16::HL) {
        hl = value;
    } else if constexpr (rr == Register16::PC) {
        pc = value;
    } else if constexpr (rr == Register16::SP) {
        sp = value;
    }
}

template <Cpu::Register16 rr>
uint16_t& Cpu::get_reg16() {
    if constexpr (rr == Register16::AF) {
        return af;
    }
    if constexpr (rr == Register16::BC) {
        return bc;
    }
    if constexpr (rr == Register16::DE) {
        return de;
    }
    if constexpr (rr == Register16::HL) {
        return hl;
    }
    if constexpr (rr == Register16::PC) {
        return pc;
    }
    if constexpr (rr == Register16::SP) {
        return sp;
    }
}

template <Cpu::Flag f>
bool Cpu::test_flag() const {
    if constexpr (f == Flag::Z) {
        return get_bit<Specs::Bits::Flags::Z>(af);
    } else if constexpr (f == Flag::N) {
        return get_bit<Specs::Bits::Flags::N>(af);
    } else if constexpr (f == Flag::H) {
        return get_bit<Specs::Bits::Flags::H>(af);
    } else if constexpr (f == Flag::C) {
        return get_bit<Specs::Bits::Flags::C>(af);
    }
}

template <Cpu::Flag f, bool y>
bool Cpu::check_flag() const {
    return test_flag<f>() == y;
}

template <Cpu::Flag f>
void Cpu::set_flag() {
    if constexpr (f == Flag::Z) {
        set_bit<Specs::Bits::Flags::Z>(af);
    } else if constexpr (f == Flag::N) {
        set_bit<Specs::Bits::Flags::N>(af);
    } else if constexpr (f == Flag::H) {
        set_bit<Specs::Bits::Flags::H>(af);
    } else if constexpr (f == Flag::C) {
        set_bit<Specs::Bits::Flags::C>(af);
    }
}

template <Cpu::Flag f>
void Cpu::set_flag(bool value) {
    if constexpr (f == Flag::Z) {
        set_bit<Specs::Bits::Flags::Z>(af, value);
    } else if constexpr (f == Flag::N) {
        set_bit<Specs::Bits::Flags::N>(af, value);
    } else if constexpr (f == Flag::H) {
        set_bit<Specs::Bits::Flags::H>(af, value);
    } else if constexpr (f == Flag::C) {
        set_bit<Specs::Bits::Flags::C>(af, value);
    }
}

template <Cpu::Flag f>
void Cpu::reset_flag() {
    if constexpr (f == Flag::Z) {
        reset_bit<Specs::Bits::Flags::Z>(af);
    } else if constexpr (f == Flag::N) {
        reset_bit<Specs::Bits::Flags::N>(af);
    } else if constexpr (f == Flag::H) {
        reset_bit<Specs::Bits::Flags::H>(af);
    } else if constexpr (f == Flag::C) {
        reset_bit<Specs::Bits::Flags::C>(af);
    }
}

// ============================= INSTRUCTIONS ==================================

void Cpu::invalid_microop() { // NOLINT(readability-make-member-function-const)
    FATAL("Invalid micro operation at address " + hex(pc));
}

void Cpu::isr_m0() {
    ASSERT(interrupt.state == InterruptState::Serving);
    idu.decrement(pc);
}

void Cpu::isr_m1() {
    idu.decrement(sp);
}

void Cpu::isr_m2() {
    write(sp, read_reg8<Register8::PC_H>());
    idu.decrement(sp);
}

void Cpu::isr_m3() {
    write(sp, read_reg8<Register8::PC_L>());

    static constexpr uint16_t IRQ_LOOKUP[] {
        0x0000, 0x0040, 0x0048, 0x0000, 0x0050, 0x0000, 0x0000, 0x0000, 0x0058,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0060,
    };

    // The reads of IE and IF happens after the high byte of PC has been pushed.
    // Note that there is an opportunity to cancel the interrupt and jump to address
    // 0x0000 if the flag in IF/IE has been reset since the trigger of the interrupt.

    uint8_t lsb = least_significant_bit(interrupts.IE & interrupts.IF);
    ASSERT(lsb <= 16);

    interrupts.IF = interrupts.IF ^ lsb;

    pc = IRQ_LOOKUP[lsb];
}

void Cpu::isr_m4() {
    ime = ImeState::Disabled;
    interrupt.state = InterruptState::None;
    fetch();
}

void Cpu::nop_m0() {
    fetch();
}

void Cpu::stop_m0() {
    // STOP behavior on DMG varies accordingly to the table below.
    //
    //   Joypad | Interrupts  ||  Mode  |   DIV   |  Instr. Length
    // ----------------------------------------------------------------
    //     0    |      0      ||  STOP  |  Reset  |       2
    //     0    |      1      ||  STOP  |  Reset  |       1
    //     1    |      0      ||  HALT  |  -----  |       2
    //     1    |      1      ||  ----  |  ------ |       1

    bool has_joypad_input = keep_bits<4>(joypad.read_p1()) == bitmask<4>;
    bool has_pending_interrupts = get_pending_interrupts();

    if (has_joypad_input) {
        stop_controller.stop();
    } else {
        halted = !has_pending_interrupts;
    }

    instruction.microop.counter = 0;

    // TODO: not sure about what happens in the hardware internally.
    if (has_pending_interrupts) {
        // Standard fetch.
        fetching = true;
    } else {
        // This effectively makes STOP use one more byte
        // as it resumes with a NOP that fetches again.
        instruction.microop.selector = nop;
    }

    read(pc);
    idu.increment(pc);
}

void Cpu::halt_m0() {
    ASSERT(!halted);
    ASSERT(interrupt.state != InterruptState::Serving);

    // HALT is entered only if there's no pending interrupt.
    halted = !get_pending_interrupts();

    // Fetch (eventually without PC increment, read below).
    instruction.microop.counter = 0;
    fetching = true;
    read(pc);

    // Handle HALT bug.
    // PC is not incremented if there's a pending interrupt (i.e. halt is not entered).
    // Despite what some documentation says, this seems to happen regardless IME state.
    if (halted) {
        idu.increment(pc);
    }
}

void Cpu::di_m0() {
    ime = ImeState::Disabled;
    fetch();
}

void Cpu::ei_m0() {
    if (ime == ImeState::Disabled) {
        ime = ImeState::Requested;
    }
    fetch();
}

// e.g. 01 | LD BC,d16

template <Cpu::Register16 rr>
void Cpu::ld_rr_uu_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register16 rr>
void Cpu::ld_rr_uu_m1() {
    lsb = io.data;
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register16 rr>
void Cpu::ld_rr_uu_m2() {
    write_reg16<rr>(concat(io.data, lsb));
    fetch();
}

// e.g. 36 | LD (HL),d8

template <Cpu::Register16 rr>
void Cpu::ld_arr_u_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register16 rr>
void Cpu::ld_arr_u_m1() {
    write(read_reg16<rr>(), io.data);
}

template <Cpu::Register16 rr>
void Cpu::ld_arr_u_m2() {
    fetch();
}

// e.g. 02 | LD (BC),A

template <Cpu::Register16 rr, Cpu::Register8 r>
void Cpu::ld_arr_r_m0() {
    write(read_reg16<rr>(), read_reg8<r>());
}

template <Cpu::Register16 rr, Cpu::Register8 r>
void Cpu::ld_arr_r_m1() {
    fetch();
}

// e.g. 22 | LD (HL+),A

template <Cpu::Register16 rr, Cpu::Register8 r, int8_t inc>
void Cpu::ld_arri_r_m0() {
    uint16_t& r16 = get_reg16<rr>();
    write(r16, read_reg8<r>());
    idu.increment<inc>(r16);
}

template <Cpu::Register16 rr, Cpu::Register8 r, int8_t inc>
void Cpu::ld_arri_r_m1() {
    fetch();
}

// e.g. 06 | LD B,d8

template <Cpu::Register8 r>
void Cpu::ld_r_u_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register8 r>
void Cpu::ld_r_u_m1() {
    write_reg8<r>(io.data);
    fetch();
}

// e.g. 08 | LD (a16),SP

template <Cpu::Register16 rr>
void Cpu::ld_ann_rr_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register16 rr>
void Cpu::ld_ann_rr_m1() {
    lsb = io.data;
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register16 rr>
void Cpu::ld_ann_rr_m2() {
    addr = concat(io.data, lsb);
    uu = read_reg16<rr>();
    write(addr, get_byte<0>(uu));
}

template <Cpu::Register16 rr>
void Cpu::ld_ann_rr_m3() {
    write(addr + 1, get_byte<1>(uu));
}

template <Cpu::Register16 rr>
void Cpu::ld_ann_rr_m4() {
    fetch();
}

// e.g. 0A |  LD A,(BC)

template <Cpu::Register8 r, Cpu::Register16 rr>
void Cpu::ld_r_arr_m0() {
    read(read_reg16<rr>());
}

template <Cpu::Register8 r, Cpu::Register16 rr>
void Cpu::ld_r_arr_m1() {
    write_reg8<r>(io.data);
    fetch();
}

// e.g. 2A |  LD A,(HL+)

template <Cpu::Register8 r, Cpu::Register16 rr, int8_t inc>
void Cpu::ld_r_arri_m0() {
    uint16_t& r16 = get_reg16<rr>();
    read(r16);
    idu.increment<inc>(r16);
}

template <Cpu::Register8 r, Cpu::Register16 rr, int8_t inc>
void Cpu::ld_r_arri_m1() {
    write_reg8<r>(io.data);
    fetch();
}

// e.g. 41 |  LD B,C

template <Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::ld_r_r_m0() {
    write_reg8<r1>(read_reg8<r2>());
    fetch();
}

// e.g. E0 | LDH (a8),A

template <Cpu::Register8 r>
void Cpu::ldh_an_r_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register8 r>
void Cpu::ldh_an_r_m1() {
    write(concat(0xFF, io.data), read_reg8<r>());
}

template <Cpu::Register8 r>
void Cpu::ldh_an_r_m2() {
    fetch();
}

// e.g. F0 | LDH A,(a8)

template <Cpu::Register8 r>
void Cpu::ldh_r_an_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register8 r>
void Cpu::ldh_r_an_m1() {
    read(concat(0xFF, io.data));
}

template <Cpu::Register8 r>
void Cpu::ldh_r_an_m2() {
    write_reg8<r>(io.data);
    fetch();
}

// e.g. E2 | LD (C),A

template <Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::ldh_ar_r_m0() {
    write(concat(0xFF, read_reg8<r1>()), read_reg8<r2>());
}

template <Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::ldh_ar_r_m1() {
    fetch();
}

// e.g. F2 | LD A,(C)

template <Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::ldh_r_ar_m0() {
    read(concat(0xFF, read_reg8<r2>()));
}

template <Cpu::Register8 r1, Cpu::Register8 r2>
void Cpu::ldh_r_ar_m1() {
    write_reg8<r1>(io.data);
    fetch();
}

// e.g. EA | LD (a16),A

template <Cpu::Register8 r>
void Cpu::ld_ann_r_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register8 r>
void Cpu::ld_ann_r_m1() {
    lsb = io.data;
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register8 r>
void Cpu::ld_ann_r_m2() {
    write(concat(io.data, lsb), read_reg8<r>());
}

template <Cpu::Register8 r>
void Cpu::ld_ann_r_m3() {
    fetch();
}

// e.g. FA | LD A,(a16)

template <Cpu::Register8 r>
void Cpu::ld_r_ann_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register8 r>
void Cpu::ld_r_ann_m1() {
    lsb = io.data;
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register8 r>
void Cpu::ld_r_ann_m2() {
    read(concat(io.data, lsb));
}

template <Cpu::Register8 r>
void Cpu::ld_r_ann_m3() {
    write_reg8<r>(io.data);
    fetch();
}

// e.g. F9 | LD SP,HL

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::ld_rr_rr_m0() {
    write_reg16<rr1>(read_reg16<rr2>());
}

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::ld_rr_rr_m1() {
    fetch();
}

// e.g. F8 | LD HL,SP+r8

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::ld_rr_rrs_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::ld_rr_rrs_m1() {
    uu = read_reg16<rr2>();
    auto [result, h, c] = sum_carry<3, 7>(uu, to_signed(io.data));
    write_reg16<rr1>(result);
    reset_flag<(Flag::Z)>();
    reset_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
    // TODO: when are flags set?
    // TODO: does it work?
}

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::ld_rr_rrs_m2() {
    fetch();
}

// e.g. 04 | INC B

template <Cpu::Register8 r>
void Cpu::inc_r_m0() {
    uint8_t tmp = read_reg8<r>();
    auto [result, h] = sum_carry<3>(tmp, 1);
    write_reg8<r>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<Flag::N>();
    set_flag<Flag::H>(h);
    fetch();
}

// e.g. 03 | INC BC

template <Cpu::Register16 rr>
void Cpu::inc_rr_m0() {
    idu.increment(get_reg16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::inc_rr_m1() {
    fetch();
}

// e.g. 34 | INC (HL)

template <Cpu::Register16 rr>
void Cpu::inc_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <Cpu::Register16 rr>
void Cpu::inc_arr_m1() {
    auto [result, h] = sum_carry<3>(io.data, 1);
    write(addr, result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
}

template <Cpu::Register16 rr>
void Cpu::inc_arr_m2() {
    fetch();
}

// e.g. 05 | DEC B

template <Cpu::Register8 r>
void Cpu::dec_r_m0() {
    uint8_t tmp = read_reg8<r>();
    auto [result, h] = sub_borrow<3>(tmp, 1);
    write_reg8<r>(result);
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    fetch();
}

// e.g. 0B | DEC BC

template <Cpu::Register16 rr>
void Cpu::dec_rr_m0() {
    idu.decrement(get_reg16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::dec_rr_m1() {
    fetch();
}

// e.g. 35 | DEC (HL)

template <Cpu::Register16 rr>
void Cpu::dec_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <Cpu::Register16 rr>
void Cpu::dec_arr_m1() {
    auto [result, h] = sub_borrow<3>(io.data, 1);
    write(addr, result);
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
}

template <Cpu::Register16 rr>
void Cpu::dec_arr_m2() {
    fetch();
}

// e.g. 80 | ADD B

template <Cpu::Register8 r>
void Cpu::add_r_m0() {
    auto [result, h, c] = sum_carry<3, 7>(read_reg8<Register8::A>(), read_reg8<r>());
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
    fetch();
}

// e.g. 86 | ADD (HL)

template <Cpu::Register16 rr>
void Cpu::add_arr_m0() {
    read(read_reg16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::add_arr_m1() {
    auto [result, h, c] = sum_carry<3, 7>(read_reg8<Register8::A>(), io.data);
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
    fetch();
}

// C6 | ADD A,d8

void Cpu::add_u_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::add_u_m1() {
    auto [result, h, c] = sum_carry<3, 7>(read_reg8<Register8::A>(), io.data);
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
    fetch();
}

// e.g. 88 | ADC B

template <Cpu::Register8 r>
void Cpu::adc_r_m0() {
    // TODO: dont like this + C very much...
    auto [tmp, h0, c0] = sum_carry<3, 7>(read_reg8<Register8::A>(), read_reg8<r>());
    auto [result, h, c] = sum_carry<3, 7>(tmp, test_flag<Flag::C>());
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    set_flag<Flag::H>(h | h0);
    set_flag<Flag::C>(c | c0);
    fetch();
}

// e.g. 8E | ADC (HL)

template <Cpu::Register16 rr>
void Cpu::adc_arr_m0() {
    read(read_reg16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::adc_arr_m1() {
    // TODO: is this ok?
    auto [tmp, h0, c0] = sum_carry<3, 7>(read_reg8<Register8::A>(), io.data);
    auto [result, h, c] = sum_carry<3, 7>(tmp, test_flag<Flag::C>());
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    set_flag<Flag::H>(h | h0);
    set_flag<Flag::C>(c | c0);
    fetch();
}

// CE | ADC A,d8

void Cpu::adc_u_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::adc_u_m1() {
    auto [tmp, h0, c0] = sum_carry<3, 7>(read_reg8<Register8::A>(), io.data);
    auto [result, h, c] = sum_carry<3, 7>(tmp, test_flag<Flag::C>());
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    set_flag<Flag::H>(h | h0);
    set_flag<Flag::C>(c | c0);
    fetch();
}

// e.g. 09 | ADD HL,BC

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::add_rr_rr_m0() {
    auto [result, h, c] = sum_carry<11, 15>(read_reg16<rr1>(), read_reg16<rr2>());
    write_reg16<rr1>(result);
    reset_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
}

template <Cpu::Register16 rr1, Cpu::Register16 rr2>
void Cpu::add_rr_rr_m1() {
    fetch();
}

// e.g. E8 | ADD SP,r8

template <Cpu::Register16 rr>
void Cpu::add_rr_s_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Register16 rr>
void Cpu::add_rr_s_m1() {
    auto [result, h, c] = sum_carry<3, 7>(read_reg16<rr>(), to_signed(io.data));
    write_reg16<rr>(result);
    reset_flag<(Flag::Z)>();
    reset_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
}

template <Cpu::Register16 rr>
void Cpu::add_rr_s_m2() {
    // TODO: why? i guess something about the instruction timing is wrong
}

template <Cpu::Register16 rr>
void Cpu::add_rr_s_m3() {
    fetch();
}

// e.g. 90 | SUB B

template <Cpu::Register8 r>
void Cpu::sub_r_m0() {
    auto [result, h, c] = sub_borrow<3, 7>(read_reg8<Register8::A>(), read_reg8<r>());
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
    fetch();
}

// e.g. 96 | SUB (HL)

template <Cpu::Register16 rr>
void Cpu::sub_arr_m0() {
    read(read_reg16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::sub_arr_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(read_reg8<Register8::A>(), io.data);
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
    fetch();
}

// D6 | SUB A,d8

void Cpu::sub_u_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::sub_u_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(read_reg8<Register8::A>(), io.data);
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
    fetch();
}

// e.g. 98 | SBC B

template <Cpu::Register8 r>
void Cpu::sbc_r_m0() {
    // TODO: is this ok?
    auto [tmp, h0, c0] = sub_borrow<3, 7>(read_reg8<Register8::A>(), read_reg8<r>());
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +test_flag<Flag::C>());
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h | h0);
    set_flag<Flag::C>(c | c0);
    fetch();
}

// e.g. 9E | SBC A,(HL)

template <Cpu::Register16 rr>
void Cpu::sbc_arr_m0() {
    read(read_reg16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::sbc_arr_m1() {
    // TODO: dont like this - C very much...
    auto [tmp, h0, c0] = sub_borrow<3, 7>(read_reg8<Register8::A>(), io.data);
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +test_flag<Flag::C>());
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h | h0);
    set_flag<Flag::C>(c | c0);
    fetch();
}

// D6 | SBC A,d8

void Cpu::sbc_u_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::sbc_u_m1() {
    auto [tmp, h0, c0] = sub_borrow<3, 7>(read_reg8<Register8::A>(), io.data);
    auto [result, h, c] = sub_borrow<3, 7>(tmp, +test_flag<Flag::C>());
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h | h0);
    set_flag<Flag::C>(c | c0);
    fetch();
}

// e.g. A0 | AND B

template <Cpu::Register8 r>
void Cpu::and_r_m0() {
    uint8_t result = read_reg8<Register8::A>() & read_reg8<r>();
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    set_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
    fetch();
}

// e.g. A6 | AND (HL)

template <Cpu::Register16 rr>
void Cpu::and_arr_m0() {
    read(read_reg16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::and_arr_m1() {
    uint8_t result = read_reg8<Register8::A>() & io.data;
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    set_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
    fetch();
}

// E6 | AND d8

void Cpu::and_u_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::and_u_m1() {
    uint8_t result = read_reg8<Register8::A>() & io.data;
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    set_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
    fetch();
}

// B0 | OR B

template <Cpu::Register8 r>
void Cpu::or_r_m0() {
    uint8_t result = read_reg8<Register8::A>() | read_reg8<r>();
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
    fetch();
}

// e.g. B6 | OR (HL)

template <Cpu::Register16 rr>
void Cpu::or_arr_m0() {
    read(read_reg16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::or_arr_m1() {
    uint8_t result = read_reg8<Register8::A>() | io.data;
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
    fetch();
}

// F6 | OR d8

void Cpu::or_u_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::or_u_m1() {
    uint8_t result = read_reg8<Register8::A>() | io.data;
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
    fetch();
}

// e.g. A8 | XOR B

template <Cpu::Register8 r>
void Cpu::xor_r_m0() {
    uint8_t result = read_reg8<Register8::A>() ^ read_reg8<r>();
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
    fetch();
}

template <Cpu::Register16 rr>
void Cpu::xor_arr_m0() {
    read(read_reg16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::xor_arr_m1() {
    uint8_t result = read_reg8<Register8::A>() ^ io.data;
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
    fetch();
}

// EE | XOR d8

void Cpu::xor_u_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::xor_u_m1() {
    uint8_t result = read_reg8<Register8::A>() ^ io.data;
    write_reg8<Register8::A>(result);
    set_flag<Flag::Z>(result == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
    fetch();
}

template <Cpu::Register8 r>
void Cpu::cp_r_m0() {
    auto [result, h, c] = sub_borrow<3, 7>(read_reg8<Register8::A>(), read_reg8<r>());
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
    fetch();
}

template <Cpu::Register16 rr>
void Cpu::cp_arr_m0() {
    read(read_reg16<rr>());
}

template <Cpu::Register16 rr>
void Cpu::cp_arr_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(read_reg8<Register8::A>(), io.data);
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
    fetch();
}

// FE | CP d8

void Cpu::cp_u_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::cp_u_m1() {
    auto [result, h, c] = sub_borrow<3, 7>(read_reg8<Register8::A>(), io.data);
    set_flag<Flag::Z>(result == 0);
    set_flag<(Flag::N)>();
    set_flag<Flag::H>(h);
    set_flag<Flag::C>(c);
    fetch();
}

// 27 | DAA

void Cpu::daa_m0() {
    u = read_reg8<Register8::A>();
    auto N = test_flag<Flag::N>();
    auto H = test_flag<Flag::H>();
    auto C = test_flag<Flag::C>();

    if (!N) {
        if (C || u > 0x99) {
            u += 0x60;
            C = true;
        }
        if (H || get_nibble<0>(u) > 0x9) {
            u += 0x06;
        }
    } else {
        if (C) {
            u -= 0x60;
        }
        if (H) {
            u -= 0x06;
        }
    }
    write_reg8<Register8::A>(u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(C);
    fetch();
}

// 37 | SCF

void Cpu::scf_m0() {
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<(Flag::C)>();
    fetch();
}

// 2F | CPL

void Cpu::cpl_m0() {
    write_reg8<Register8::A>(~read_reg8<Register8::A>());
    set_flag<(Flag::N)>();
    set_flag<(Flag::H)>();
    fetch();
}

// 3F | CCF_m0

void Cpu::ccf_m0() {
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(!test_flag<Flag::C>());
    fetch();
}

// 07 | RLCA

void Cpu::rlca_m0() {
    u = read_reg8<Register8::A>();
    bool b7 = test_bit<7>(u);
    u = (u << 1) | b7;
    write_reg8<Register8::A>(u);
    reset_flag<(Flag::Z)>();
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b7);
    fetch();
}

// 17 | RLA

void Cpu::rla_m0() {
    u = read_reg8<Register8::A>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | test_flag<Flag::C>();
    write_reg8<Register8::A>(u);
    reset_flag<(Flag::Z)>();
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b7);
    fetch();
}

// 0F | RRCA

void Cpu::rrca_m0() {
    u = read_reg8<Register8::A>();
    bool b0 = test_bit<0>(u);
    u = (u >> 1) | (b0 << 7);
    write_reg8<Register8::A>(u);
    reset_flag<(Flag::Z)>();
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b0);
    fetch();
}

// 1F | RRA

void Cpu::rra_m0() {
    u = read_reg8<Register8::A>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (test_flag<Flag::C>() << 7);
    write_reg8<Register8::A>(u);
    reset_flag<(Flag::Z)>();
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b0);
    fetch();
}

// e.g. 18 | JR r8

void Cpu::jr_s_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::jr_s_m1() {
    pc = (int16_t)pc + to_signed(io.data);
}

void Cpu::jr_s_m2() {
    fetch();
}

// e.g. 28 | JR Z,r8
// e.g. 20 | JR NZ,r8

template <Cpu::Flag f, bool y>
void Cpu::jr_c_s_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Flag f, bool y>
void Cpu::jr_c_s_m1() {
    if (check_flag<f, y>()) {
        pc = pc + to_signed(io.data);
    } else {
        fetch();
    }
}

template <Cpu::Flag f, bool y>
void Cpu::jr_c_s_m2() {
    fetch();
}

// e.g. C3 | JP a16

void Cpu::jp_uu_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::jp_uu_m1() {
    lsb = io.data;
    read(pc);
    idu.increment(pc);
}

void Cpu::jp_uu_m2() {
    pc = concat(io.data, lsb);
}

void Cpu::jp_uu_m3() {
    fetch();
}

// e.g. E9 | JP (HL)

template <Cpu::Register16 rr>
void Cpu::jp_rr_m0() {
    pc = read_reg16<rr>();
    fetch();
}

// e.g. CA | JP Z,a16
// e.g. C2 | JP NZ,a16

template <Cpu::Flag f, bool y>
void Cpu::jp_c_uu_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Flag f, bool y>
void Cpu::jp_c_uu_m1() {
    lsb = io.data;
    read(pc);
    idu.increment(pc);
}

template <Cpu::Flag f, bool y>
void Cpu::jp_c_uu_m2() {
    if (check_flag<f, y>()) {
        pc = concat(io.data, lsb);
    } else {
        fetch();
    }
}

template <Cpu::Flag f, bool y>
void Cpu::jp_c_uu_m3() {
    fetch();
}

// CD | CALL a16

void Cpu::call_uu_m0() {
    read(pc);
    idu.increment(pc);
}

void Cpu::call_uu_m1() {
    lsb = io.data;
    read(pc);
    idu.increment(pc);
}

void Cpu::call_uu_m2() {
    msb = io.data;
    idu.decrement(sp);
}

void Cpu::call_uu_m3() {
    write(sp, read_reg8<Register8::PC_H>());
    idu.decrement(sp);
}

void Cpu::call_uu_m4() {
    write(sp, read_reg8<Register8::PC_L>());
    pc = concat(msb, lsb);
}

void Cpu::call_uu_m5() {
    fetch();
}

// e.g. C4 | CALL NZ,a16

template <Cpu::Flag f, bool y>
void Cpu::call_c_uu_m0() {
    read(pc);
    idu.increment(pc);
}

template <Cpu::Flag f, bool y>
void Cpu::call_c_uu_m1() {
    lsb = io.data;
    read(pc);
    idu.increment(pc);
}
template <Cpu::Flag f, bool y>
void Cpu::call_c_uu_m2() {
    if (!check_flag<f, y>()) {
        fetch();
    } else {
        msb = io.data;
        idu.decrement(sp);
    }
}

template <Cpu::Flag f, bool y>
void Cpu::call_c_uu_m3() {
    write(sp, read_reg8<Register8::PC_H>());
    idu.decrement(sp);
}

template <Cpu::Flag f, bool y>
void Cpu::call_c_uu_m4() {
    write(sp, read_reg8<Register8::PC_L>());
    pc = concat(msb, lsb);
}

template <Cpu::Flag f, bool y>
void Cpu::call_c_uu_m5() {
    fetch();
}

// C7 | RST 00H

template <uint8_t n>
void Cpu::rst_m0() {
    idu.decrement(sp);
}

template <uint8_t n>
void Cpu::rst_m1() {
    write(sp, read_reg8<Register8::PC_H>());
    idu.decrement(sp);
}

template <uint8_t n>
void Cpu::rst_m2() {
    write(sp, read_reg8<Register8::PC_L>());
    pc = n;
}

template <uint8_t n>
void Cpu::rst_m3() {
    fetch();
}

// C9 | RET

void Cpu::ret_uu_m0() {
    read(sp);
    idu.increment(sp);
}

void Cpu::ret_uu_m1() {
    lsb = io.data;
    read(sp);
    idu.increment(sp);
}

void Cpu::ret_uu_m2() {
    pc = concat(io.data, lsb);
}

void Cpu::ret_uu_m3() {
    fetch();
}

// D9 | RETI

void Cpu::reti_uu_m0() {
    read(sp);
    idu.increment(sp);
}

void Cpu::reti_uu_m1() {
    lsb = io.data;
    read(sp);
    idu.increment(sp);
}

void Cpu::reti_uu_m2() {
    pc = concat(io.data, lsb);
    ime = ImeState::Enabled;
}

void Cpu::reti_uu_m3() {
    fetch();
}

// e.g. C0 | RET NZ

template <Cpu::Flag f, bool y>
void Cpu::ret_c_uu_m0() {
}

template <Cpu::Flag f, bool y>
void Cpu::ret_c_uu_m1() {
    if (check_flag<f, y>()) {
        read(sp);
        idu.increment(sp);
    } else {
        fetch();
    }
}

template <Cpu::Flag f, bool y>
void Cpu::ret_c_uu_m2() {
    lsb = io.data;
    read(sp);
    idu.increment(sp);
}

template <Cpu::Flag f, bool y>
void Cpu::ret_c_uu_m3() {
    pc = concat(io.data, lsb);
}

template <Cpu::Flag f, bool y>
void Cpu::ret_c_uu_m4() {
    fetch();
}

// e.g. C5 | PUSH BC

template <Cpu::Register16 rr>
void Cpu::push_rr_m0() {
    uu = read_reg16<rr>();
    idu.decrement(sp);
}

template <Cpu::Register16 rr>
void Cpu::push_rr_m1() {
    write(sp, get_byte<1>(uu));
    idu.decrement(sp);
}

template <Cpu::Register16 rr>
void Cpu::push_rr_m2() {
    write(sp, get_byte<0>(uu));
}

template <Cpu::Register16 rr>
void Cpu::push_rr_m3() {
    fetch();
}

// e.g. C1 | POP BC

template <Cpu::Register16 rr>
void Cpu::pop_rr_m0() {
    read(sp);
    idu.increment(sp);
}

template <Cpu::Register16 rr>
void Cpu::pop_rr_m1() {
    lsb = io.data;
    read(sp);
    idu.increment(sp);
    // TODO: figure out if this m-cycle counts as read + inc or as read only for the OAM bug
}

template <Cpu::Register16 rr>
void Cpu::pop_rr_m2() {
    write_reg16<rr>(concat(io.data, lsb));
    fetch();
}

void Cpu::cb_m0() {
    fetch_cb();
}

// e.g. CB 00 | RLC B

template <Cpu::Register8 r>
void Cpu::rlc_r_m0() {
    u = read_reg8<r>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | b7;
    write_reg8<r>(u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b7);
    fetch();
}

// e.g. CB 06 | RLC (HL)

template <Cpu::Register16 rr>
void Cpu::rlc_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <Cpu::Register16 rr>
void Cpu::rlc_arr_m1() {
    bool b7 = get_bit<7>(io.data);
    u = (io.data << 1) | b7;
    write(addr, u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b7);
}

template <Cpu::Register16 rr>
void Cpu::rlc_arr_m2() {
    fetch();
}

// e.g. CB 08 | RRC B

template <Cpu::Register8 r>
void Cpu::rrc_r_m0() {
    u = read_reg8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (b0 << 7);
    write_reg8<r>(u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b0);
    fetch();
}

// e.g. CB 0E | RRC (HL)

template <Cpu::Register16 rr>
void Cpu::rrc_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <Cpu::Register16 rr>
void Cpu::rrc_arr_m1() {
    bool b0 = get_bit<0>(io.data);
    u = (io.data >> 1) | (b0 << 7);
    write(addr, u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b0);
}

template <Cpu::Register16 rr>
void Cpu::rrc_arr_m2() {
    fetch();
}

// e.g. CB 10 | RL B

template <Cpu::Register8 r>
void Cpu::rl_r_m0() {
    u = read_reg8<r>();
    bool b7 = get_bit<7>(u);
    u = (u << 1) | test_flag<Flag::C>();
    write_reg8<r>(u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b7);
    fetch();
}

// e.g. CB 16 | RL (HL)

template <Cpu::Register16 rr>
void Cpu::rl_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <Cpu::Register16 rr>
void Cpu::rl_arr_m1() {
    bool b7 = get_bit<7>(io.data);
    u = (io.data << 1) | test_flag<Flag::C>();
    write(addr, u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b7);
}

template <Cpu::Register16 rr>
void Cpu::rl_arr_m2() {
    fetch();
}

// e.g. CB 18 | RR B

template <Cpu::Register8 r>
void Cpu::rr_r_m0() {
    u = read_reg8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (test_flag<Flag::C>() << 7);
    write_reg8<r>(u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b0);
    fetch();
}

// e.g. CB 1E | RR (HL)

template <Cpu::Register16 rr>
void Cpu::rr_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <Cpu::Register16 rr>
void Cpu::rr_arr_m1() {
    bool b0 = get_bit<0>(io.data);
    u = (io.data >> 1) | (test_flag<Flag::C>() << 7);
    write(addr, u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b0);
}

template <Cpu::Register16 rr>
void Cpu::rr_arr_m2() {
    fetch();
}

// e.g. CB 28 | SRA B

template <Cpu::Register8 r>
void Cpu::sra_r_m0() {
    u = read_reg8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1) | (u & bit<7>);
    write_reg8<r>(u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b0);
    fetch();
}

// e.g. CB 2E | SRA (HL)

template <Cpu::Register16 rr>
void Cpu::sra_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <Cpu::Register16 rr>
void Cpu::sra_arr_m1() {
    bool b0 = get_bit<0>(io.data);
    u = (io.data >> 1) | (io.data & bit<7>);
    write(addr, u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b0);
}

template <Cpu::Register16 rr>
void Cpu::sra_arr_m2() {
    fetch();
}

// e.g. CB 38 | SRL B

template <Cpu::Register8 r>
void Cpu::srl_r_m0() {
    u = read_reg8<r>();
    bool b0 = get_bit<0>(u);
    u = (u >> 1);
    write_reg8<r>(u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b0);
    fetch();
}

// e.g. CB 3E | SRL (HL)

template <Cpu::Register16 rr>
void Cpu::srl_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <Cpu::Register16 rr>
void Cpu::srl_arr_m1() {
    bool b0 = get_bit<0>(io.data);
    u = (io.data >> 1);
    write(addr, u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b0);
}

template <Cpu::Register16 rr>
void Cpu::srl_arr_m2() {
    fetch();
}

// e.g. CB 20 | SLA B

template <Cpu::Register8 r>
void Cpu::sla_r_m0() {
    u = read_reg8<r>();
    bool b7 = get_bit<7>(u);
    u = u << 1;
    write_reg8<r>(u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b7);
    fetch();
}

// e.g. CB 26 | SLA (HL)

template <Cpu::Register16 rr>
void Cpu::sla_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <Cpu::Register16 rr>
void Cpu::sla_arr_m1() {
    bool b7 = get_bit<7>(io.data);
    u = io.data << 1;
    write(addr, u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    set_flag<Flag::C>(b7);
}

template <Cpu::Register16 rr>
void Cpu::sla_arr_m2() {
    fetch();
}

// e.g. CB 30 | SWAP B

template <Cpu::Register8 r>
void Cpu::swap_r_m0() {
    u = read_reg8<r>();
    u = ((u & 0x0F) << 4) | ((u & 0xF0) >> 4);
    write_reg8<r>(u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
    fetch();
}

// e.g. CB 36 | SWAP (HL)

template <Cpu::Register16 rr>
void Cpu::swap_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <Cpu::Register16 rr>
void Cpu::swap_arr_m1() {
    u = ((io.data & 0x0F) << 4) | ((io.data & 0xF0) >> 4);
    write(addr, u);
    set_flag<Flag::Z>(u == 0);
    reset_flag<(Flag::N)>();
    reset_flag<(Flag::H)>();
    reset_flag<(Flag::C)>();
}

template <Cpu::Register16 rr>
void Cpu::swap_arr_m2() {
    fetch();
}

// e.g. CB 40 | BIT 0,B

template <uint8_t n, Cpu::Register8 r>
void Cpu::bit_r_m0() {
    b = get_bit<n>(read_reg8<r>());
    set_flag<Flag::Z>(b == 0);
    reset_flag<(Flag::N)>();
    set_flag<(Flag::H)>();
    fetch();
}

// e.g. CB 46 | BIT 0,(hl)

template <uint8_t n, Cpu::Register16 rr>
void Cpu::bit_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <uint8_t n, Cpu::Register16 rr>
void Cpu::bit_arr_m1() {
    b = test_bit<n>(io.data);
    set_flag<Flag::Z>(b == 0);
    reset_flag<(Flag::N)>();
    set_flag<(Flag::H)>();
    fetch();
}

// e.g. CB 80 | RES 0,B

template <uint8_t n, Cpu::Register8 r>
void Cpu::res_r_m0() {
    u = read_reg8<r>();
    reset_bit<n>(u);
    write_reg8<r>(u);
    fetch();
}

// e.g. CB 86 | RES 0,(hl)

template <uint8_t n, Cpu::Register16 rr>
void Cpu::res_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <uint8_t n, Cpu::Register16 rr>
void Cpu::res_arr_m1() {
    reset_bit<n>(io.data);
    write(addr, io.data);
}

template <uint8_t n, Cpu::Register16 rr>
void Cpu::res_arr_m2() {
    fetch();
}

// e.g. CB C0 | SET 0,B

template <uint8_t n, Cpu::Register8 r>
void Cpu::set_r_m0() {
    u = read_reg8<r>();
    set_bit<n>(u);
    write_reg8<r>(u);
    fetch();
}

// e.g. CB C6 | SET 0,(HL)

template <uint8_t n, Cpu::Register16 rr>
void Cpu::set_arr_m0() {
    addr = read_reg16<rr>();
    read(addr);
}

template <uint8_t n, Cpu::Register16 rr>
void Cpu::set_arr_m1() {
    set_bit<n>(io.data);
    write(addr, io.data);
}

template <uint8_t n, Cpu::Register16 rr>
void Cpu::set_arr_m2() {
    fetch();
}

#ifdef ENABLE_ASSERTS
template <uint8_t t>
void Cpu::assert_tick() {
    ASSERT(last_tick == t);
    last_tick = mod<4>(t + 1);
}
#endif
