#ifndef CPU_H
#define CPU_H

#include <cstdint>

#include "docboy/common/macros.h"
#include "docboy/mmu/mmu.h"

class Idu;
class Interrupts;
class Joypad;
class StopController;
class SpeedSwitch;
class SpeedSwitchController;
class Parcel;

class Cpu {
    DEBUGGABLE_CLASS()
    TESTABLE_CLASS()

public:
#ifdef ENABLE_CGB
    Cpu(Idu& idu, Interrupts& interrupts, Mmu::View<Device::Cpu> mmu, Joypad& joypad, bool& fetching, bool& halted,
        StopController& stop_controller, SpeedSwitch& speed_switch, SpeedSwitchController& speed_switch_controller);
#else
    Cpu(Idu& idu, Interrupts& interrupts, Mmu::View<Device::Cpu> mmu, Joypad& joypad, bool& fetching, bool& halted,
        StopController& stop_controller);
#endif

    void tick_t0();
    void tick_t1();
    void tick_t2();
    void tick_t3();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    enum class Register8 : uint8_t {
        A,
        B,
        C,
        D,
        E,
        F,
        H,
        L,
        SP_H,
        SP_L,
        PC_H,
        PC_L,
    };

    enum class Register16 : uint8_t {
        AF,
        BC,
        DE,
        HL,
        PC,
        SP,
    };

    enum class Flag : uint8_t {
        Z,
        N,
        H,
        C,
    };

    enum class InterruptState : uint8_t {
        None,
        Pending,
        Serving,
    };

    enum class ImeState : uint8_t {
        Disabled,
        Requested,
        Pending,
        Enabled,
    };

    static constexpr uint8_t INSTR_LEN = 6;

    using MicroOperation = void (Cpu::*)();
    using Instruction = MicroOperation[INSTR_LEN];

    void tick_microop();

    void tick_even();
    void tick_odd();

    void tick_single_speed_0();
    void tick_single_speed_1();
    void tick_single_speed_2();
    void tick_single_speed_3();

    void tick_double_speed_0();
    void tick_double_speed_1();

    template <uint8_t t>
    void check_interrupt();

    uint8_t get_pending_interrupts() const;

    void serve_interrupt();

    void fetch();
    void fetch_cb();

    void read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    void flush_read();
    void flush_write();

    template <Register8 r>
    uint8_t read_reg8() const;

    template <Register8 r>
    void write_reg8(uint8_t value);

    template <Register16 rr>
    uint16_t read_reg16() const;

    template <Register16 rr>
    void write_reg16(uint16_t value);

    template <Register16 rr>
    uint16_t& get_reg16();

    template <Flag f>
    bool test_flag() const;

    template <Flag f, bool y>
    bool check_flag() const;

    template <Flag f>
    void set_flag();

    template <Flag f>
    void set_flag(bool value);

    template <Flag f>
    void reset_flag();

    /*
     * [INSTRUCTIONS LEGEND]
     * r: 8 bit register
     * rr: 16 bit register
     * rrs: 16 bit register, signed
     * n: 8 bit immediate number
     * nn: 16 bit immediate number
     * s: 8 bit immediate number, signed
     * ar: address specified by 8 bit register
     * arr: address specified by 16 bit register
     * arri: address specified by 16 bit register, then increment 16 bit register
     * an: address specified by 8 bit immediate number
     * ann: address specified by 16 bit immediate number
     * c: check flag condition
     */
    void invalid_microop();

    void isr_m0();
    void isr_m1();
    void isr_m2();
    void isr_m3();
    void isr_m4();

    void nop_m0();
    void stop_m0();
    void halt_m0();
    void di_m0();
    void ei_m0();

    template <Register16 rr>
    void ld_rr_uu_m0();
    template <Register16 rr>
    void ld_rr_uu_m1();
    template <Register16 rr>
    void ld_rr_uu_m2();

    template <Register16 rr>
    void ld_arr_u_m0();
    template <Register16 rr>
    void ld_arr_u_m1();
    template <Register16 rr>
    void ld_arr_u_m2();

    template <Register16 rr, Register8 r>
    void ld_arr_r_m0();
    template <Register16 rr, Register8 r>
    void ld_arr_r_m1();

    template <Register16 rr, Register8 r, int8_t inc>
    void ld_arri_r_m0();
    template <Register16 rr, Register8 r, int8_t inc>
    void ld_arri_r_m1();

    template <Register8 r>
    void ld_r_u_m0();
    template <Register8 r>
    void ld_r_u_m1();

    template <Register16 rr>
    void ld_ann_rr_m0();
    template <Register16 rr>
    void ld_ann_rr_m1();
    template <Register16 rr>
    void ld_ann_rr_m2();
    template <Register16 rr>
    void ld_ann_rr_m3();
    template <Register16 rr>
    void ld_ann_rr_m4();

    template <Register8 r, Register16 rr>
    void ld_r_arr_m0();
    template <Register8 r, Register16 rr>
    void ld_r_arr_m1();

    template <Register8 r, Register16 rr, int8_t inc>
    void ld_r_arri_m0();
    template <Register8 r, Register16 rr, int8_t inc>
    void ld_r_arri_m1();

    template <Register8 r1, Register8 r2>
    void ld_r_r_m0();

    template <Register8 r>
    void ldh_an_r_m0();
    template <Register8 r>
    void ldh_an_r_m1();
    template <Register8 r>
    void ldh_an_r_m2();

    template <Register8 r>
    void ldh_r_an_m0();
    template <Register8 r>
    void ldh_r_an_m1();
    template <Register8 r>
    void ldh_r_an_m2();

    template <Register8 r1, Register8 r2>
    void ldh_ar_r_m0();
    template <Register8 r1, Register8 r2>
    void ldh_ar_r_m1();

    template <Register8 r1, Register8 r2>
    void ldh_r_ar_m0();
    template <Register8 r1, Register8 r2>
    void ldh_r_ar_m1();

    template <Register8 r>
    void ld_ann_r_m0();
    template <Register8 r>
    void ld_ann_r_m1();
    template <Register8 r>
    void ld_ann_r_m2();
    template <Register8 r>
    void ld_ann_r_m3();

    template <Register8 r>
    void ld_r_ann_m0();
    template <Register8 r>
    void ld_r_ann_m1();
    template <Register8 r>
    void ld_r_ann_m2();
    template <Register8 r>
    void ld_r_ann_m3();

    template <Register16 rr1, Register16 rr2>
    void ld_rr_rr_m0();
    template <Register16 rr1, Register16 rr2>
    void ld_rr_rr_m1();

    template <Register16 rr1, Register16 rr2>
    void ld_rr_rrs_m0();
    template <Register16 rr1, Register16 rr2>
    void ld_rr_rrs_m1();
    template <Register16 rr1, Register16 rr2>
    void ld_rr_rrs_m2();

    template <Register8 r>
    void inc_r_m0();

    template <Register16 rr>
    void inc_rr_m0();
    template <Register16 rr>
    void inc_rr_m1();

    template <Register16 rr>
    void inc_arr_m0();
    template <Register16 rr>
    void inc_arr_m1();
    template <Register16 rr>
    void inc_arr_m2();

    template <Register8 r>
    void dec_r_m0();

    template <Register16 rr>
    void dec_rr_m0();
    template <Register16 rr>
    void dec_rr_m1();

    template <Register16 rr>
    void dec_arr_m0();
    template <Register16 rr>
    void dec_arr_m1();
    template <Register16 rr>
    void dec_arr_m2();

    template <Register8 r>
    void add_r_m0();

    template <Register16 rr>
    void add_arr_m0();
    template <Register16 r2>
    void add_arr_m1();

    template <Register8 r>
    void adc_r_m0();

    template <Register16 rr>
    void adc_arr_m0();
    template <Register16 r2>
    void adc_arr_m1();

    template <Register16 rr1, Register16 rr2>
    void add_rr_rr_m0();
    template <Register16 rr1, Register16 rr2>
    void add_rr_rr_m1();

    template <Register16 rr>
    void add_rr_s_m0();
    template <Register16 rr>
    void add_rr_s_m1();
    template <Register16 rr>
    void add_rr_s_m2();
    template <Register16 rr>
    void add_rr_s_m3();

    void add_u_m0();
    void add_u_m1();

    void adc_u_m0();
    void adc_u_m1();

    template <Register8 r>
    void sub_r_m0();

    template <Register16 rr>
    void sub_arr_m0();
    template <Register16 r2>
    void sub_arr_m1();

    template <Register8 r>
    void sbc_r_m0();

    template <Register16 rr>
    void sbc_arr_m0();
    template <Register16 r2>
    void sbc_arr_m1();

    void sub_u_m0();
    void sub_u_m1();

    void sbc_u_m0();
    void sbc_u_m1();

    template <Register8 r>
    void and_r_m0();

    template <Register16 rr>
    void and_arr_m0();
    template <Register16 rr>
    void and_arr_m1();

    void and_u_m0();
    void and_u_m1();

    template <Register8 r>
    void or_r_m0();

    template <Register16 rr>
    void or_arr_m0();
    template <Register16 rr>
    void or_arr_m1();

    void or_u_m0();
    void or_u_m1();

    template <Register8 r>
    void xor_r_m0();

    template <Register16 rr>
    void xor_arr_m0();
    template <Register16 rr>
    void xor_arr_m1();

    void xor_u_m0();
    void xor_u_m1();

    template <Register8 r>
    void cp_r_m0();

    template <Register16 rr>
    void cp_arr_m0();
    template <Register16 rr>
    void cp_arr_m1();

    void cp_u_m0();
    void cp_u_m1();

    void daa_m0();
    void scf_m0();

    void cpl_m0();
    void ccf_m0();

    void rlca_m0();
    void rla_m0();

    void rrca_m0();
    void rra_m0();

    void jr_s_m0();
    void jr_s_m1();
    void jr_s_m2();

    template <Flag f, bool y = true>
    void jr_c_s_m0();
    template <Flag f, bool y = true>
    void jr_c_s_m1();
    template <Flag f, bool y = true>
    void jr_c_s_m2();

    void jp_uu_m0();
    void jp_uu_m1();
    void jp_uu_m2();
    void jp_uu_m3();

    template <Register16>
    void jp_rr_m0();

    template <Flag f, bool y = true>
    void jp_c_uu_m0();
    template <Flag f, bool y = true>
    void jp_c_uu_m1();
    template <Flag f, bool y = true>
    void jp_c_uu_m2();
    template <Flag f, bool y = true>
    void jp_c_uu_m3();

    void call_uu_m0();
    void call_uu_m1();
    void call_uu_m2();
    void call_uu_m3();
    void call_uu_m4();
    void call_uu_m5();

    template <Flag f, bool y = true>
    void call_c_uu_m0();
    template <Flag f, bool y = true>
    void call_c_uu_m1();
    template <Flag f, bool y = true>
    void call_c_uu_m2();
    template <Flag f, bool y = true>
    void call_c_uu_m3();
    template <Flag f, bool y = true>
    void call_c_uu_m4();
    template <Flag f, bool y = true>
    void call_c_uu_m5();

    template <uint8_t n>
    void rst_m0();
    template <uint8_t n>
    void rst_m1();
    template <uint8_t n>
    void rst_m2();
    template <uint8_t n>
    void rst_m3();

    void ret_uu_m0();
    void ret_uu_m1();
    void ret_uu_m2();
    void ret_uu_m3();

    void reti_uu_m0();
    void reti_uu_m1();
    void reti_uu_m2();
    void reti_uu_m3();

    template <Flag f, bool y = true>
    void ret_c_uu_m0();
    template <Flag f, bool y = true>
    void ret_c_uu_m1();
    template <Flag f, bool y = true>
    void ret_c_uu_m2();
    template <Flag f, bool y = true>
    void ret_c_uu_m3();
    template <Flag f, bool y = true>
    void ret_c_uu_m4();

    template <Register16 rr>
    void push_rr_m0();
    template <Register16 rr>
    void push_rr_m1();
    template <Register16 rr>
    void push_rr_m2();
    template <Register16 rr>
    void push_rr_m3();

    template <Register16 rr>
    void pop_rr_m0();
    template <Register16 rr>
    void pop_rr_m1();
    template <Register16 rr>
    void pop_rr_m2();

    void cb_m0();

    template <Register8 r>
    void rlc_r_m0();

    template <Register16 rr>
    void rlc_arr_m0();
    template <Register16 rr>
    void rlc_arr_m1();
    template <Register16 rr>
    void rlc_arr_m2();

    template <Register8 r>
    void rrc_r_m0();

    template <Register16 rr>
    void rrc_arr_m0();
    template <Register16 rr>
    void rrc_arr_m1();
    template <Register16 rr>
    void rrc_arr_m2();

    template <Register8 r>
    void rl_r_m0();

    template <Register16 rr>
    void rl_arr_m0();
    template <Register16 rr>
    void rl_arr_m1();
    template <Register16 rr>
    void rl_arr_m2();

    template <Register8 r>
    void rr_r_m0();

    template <Register16 rr>
    void rr_arr_m0();
    template <Register16 rr>
    void rr_arr_m1();
    template <Register16 rr>
    void rr_arr_m2();

    template <Register8 r>
    void sla_r_m0();

    template <Register16 rr>
    void sla_arr_m0();
    template <Register16 rr>
    void sla_arr_m1();
    template <Register16 rr>
    void sla_arr_m2();

    template <Register8 r>
    void sra_r_m0();

    template <Register16 rr>
    void sra_arr_m0();
    template <Register16 rr>
    void sra_arr_m1();
    template <Register16 rr>
    void sra_arr_m2();

    template <Register8 r>
    void srl_r_m0();

    template <Register16 rr>
    void srl_arr_m0();
    template <Register16 rr>
    void srl_arr_m1();
    template <Register16 rr>
    void srl_arr_m2();

    template <Register8 r>
    void swap_r_m0();

    template <Register16 rr>
    void swap_arr_m0();
    template <Register16 rr>
    void swap_arr_m1();
    template <Register16 rr>
    void swap_arr_m2();

    template <uint8_t n, Register8 r>
    void bit_r_m0();

    template <uint8_t n, Register16 r>
    void bit_arr_m0();
    template <uint8_t n, Register16 r>
    void bit_arr_m1();

    template <uint8_t n, Register8 r>
    void res_r_m0();

    template <uint8_t n, Register16 r>
    void res_arr_m0();
    template <uint8_t n, Register16 r>
    void res_arr_m1();
    template <uint8_t n, Register16 r>
    void res_arr_m2();

    template <uint8_t n, Register8 r>
    void set_r_m0();

    template <uint8_t n, Register16 r>
    void set_arr_m0();
    template <uint8_t n, Register16 r>
    void set_arr_m1();
    template <uint8_t n, Register16 r>
    void set_arr_m2();

#ifdef ENABLE_DEBUGGER
    void update_debug_information();
#endif

#ifdef ENABLE_TESTS
    void update_test_information();
#endif

    Idu& idu;
    Interrupts& interrupts;
    Mmu::View<Device::Cpu> mmu;
    Joypad& joypad;
    bool& fetching;
    bool& halted;
    StopController& stop_controller;
#ifdef ENABLE_CGB
    // TODO: architectural: should CPU know them?
    SpeedSwitch& speed_switch;
    SpeedSwitchController& speed_switch_controller;
#endif

    Instruction instructions[256];
    Instruction instructions_cb[256];
    Instruction isr;

    Instruction& nop {instructions[0]};

    uint16_t af {};
    uint16_t bc {};
    uint16_t de {};
    uint16_t hl {};
    uint16_t pc {};
    uint16_t sp {};

    ImeState ime {};

    uint8_t phase {};

    struct {
        InterruptState state {};
        uint8_t remaining_ticks {};
    } interrupt;

    struct {
        struct {
            MicroOperation* selector {};
            uint8_t counter {};
        } microop;

#ifdef ENABLE_TESTS
        uint8_t opcode {};
#endif

#ifdef ENABLE_DEBUGGER
        uint16_t address {};
        uint8_t cycle_microop {};
#endif
    } instruction {};

    struct IO {
        enum class State : uint8_t {
            Idle = 0,
            Read = 1,
            Write = 2,
        };

        State state {};
        uint8_t data {};
    } io;

    bool fetching_cb {};

    // scratchpad
    struct {
        bool b {};
        uint8_t u {};
        uint8_t u2 {};
        uint8_t lsb {};
        uint8_t msb {};
        uint16_t uu {};
        uint16_t addr {};
    };

#ifdef ENABLE_DEBUGGER
    uint64_t cycles {};
#endif

#ifdef ENABLE_ASSERTS
    uint8_t last_tick {3};
#endif
};

#endif // CPU_H