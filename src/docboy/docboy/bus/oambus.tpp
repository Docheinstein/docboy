#include "docboy/memory/oam.h"
#include "oambus.h"
#include "utils/hexdump.h"

template <Device::Type Dev>
void OamBus::View<Dev>::read_word_request(uint16_t addr) {
    this->bus.template read_word_request<Dev>(addr);
}

template <Device::Type Dev>
OamBus::Word OamBus::View<Dev>::flush_read_word_request() {
    return this->bus.template flush_read_word_request<Dev>();
}

template <Device::Type Dev>
void OamBus::flush_write_request(uint8_t value) {
    if constexpr (Dev == Device::Cpu) {
        mcycle_write.happened = true;
        mcycle_write.previous_data = oam[address - Specs::MemoryLayout::OAM::START];
    }
    // TODO: else: what if DMA is writing instead?

    VideoBus::flush_write_request<Dev>(value);
}

template <Device::Type Dev>
void OamBus::clear_write_request() {
    // TODO: this seems incorrect from an hardware point of view: consider use decay time
    reset_bit<W<Dev>>(requests);
}

template <Device::Type Dev>
void OamBus::read_word_request(uint16_t addr) {
    static_assert(Dev == Device::Ppu);

    set_bit<R<Dev>>(requests);

    // If PPU is reading while CPU is accessing OAM, OAM Bug Corruption might happen.
    // There are several corruption patterns based on the following conditions:
    // - whether CPU is reading/writing
    // - the address read/write by CPU
    // - the address read by PPU
    //
    // The following patterns have been tested on a real DMG, but note that
    // some of them are not deterministic at all even on real hardware.

    const uint8_t oam_addr = addr - Specs::MemoryLayout::OAM::START;
    const uint8_t row_addr = discard_bits<3>(oam_addr + 4);

    if (oam_addr == 0x00 && mcycle_write.happened) {
        if (address & 0x04) {
            if (address & 0x02) {
                if (address & 0x01) {
                    oam_bug_write_00_7();
                } else {
                    oam_bug_write_00_6();
                }
            } else {
                if (address & 0x01) {
                    oam_bug_write_00_5();
                } else {
                    oam_bug_write_00_4();
                }
            }
        } else {
            if (address & 0x02) {
                if (address & 0x01) {
                    oam_bug_write_00_3();
                } else {
                    oam_bug_write_00_2();
                }
            } else {
                oam_bug_write_00_0();
            }
        }
    } else if (test_bit<R<Device::Cpu>>(requests)) {
        if (oam_addr == 0x00) {
            oam_bug_read_00();
        } else if (mod<16>(oam_addr) == 12) {
            if (oam_addr == 0x9C) {
                if (address & 0x04) {
                    if (address & 0x02) {
                        oam_bug_read_9c_6();
                    } else {
                        oam_bug_read_9c_0_4();
                    }
                } else {
                    if (address & 0x02) {
                        oam_bug_read_9c_2();
                    } else {
                        oam_bug_read_9c_0_4();
                    }
                }
            } else {
                oam_bug_read_write(row_addr);
            }
        } else {
            oam_bug_read(row_addr);
        }
    } else if (test_bit<W<Device::Cpu>>(requests) || test_bit<W<Device::Idu>>(requests)) {
        if (oam_addr == 0x9C) {
            if (test_bit<W<Device::Cpu>>(requests)) {
                if (address & 0x04) {
                    if (address & 0x02) {
                        oam_bug_write_9c_6();
                    } else {
                        oam_bug_write_9c_4();
                    }
                } else {
                    if (address & 0x02) {
                        oam_bug_write_9c_2();
                    } else {
                        oam_bug_write_9c_0();
                    }
                }
            }
        } else {
            oam_bug_write(row_addr);
        }
    }

    // PPU does not overwrite the address in the address bus.
    // i.e. if DMA write is in progress we end up reading from such address instead.
    // [hacktix/strikethrough]
    if (!test_bits_or<W<Device::Cpu>, W<Device::Dma>, W<Device::Idu>>(requests)) {
        address = addr;
    }
}

template <Device::Type Dev>
OamBus::Word OamBus::flush_read_word_request() {
    static_assert(Dev == Device::Ppu);

    reset_bit<R<Dev>>(requests);

    // Discard the last bit: base address for read word is always even.
    uint16_t oam_address = discard_bits<0>(address - Specs::MemoryLayout::OAM::START);

    return {oam[oam_address], oam[oam_address + 1]};
}

inline void OamBus::tick_t2() {
    mcycle_write.happened = false;
    tick();
}

inline uint16_t OamBus::read_word(const uint8_t addr) const {
    ASSERT(mod<2>(addr) == 0);
    return concat(oam[addr], oam[addr + 1]);
}

inline void OamBus::write_word(const uint8_t addr, uint16_t word) {
    ASSERT(mod<2>(addr) == 0);
    oam[addr] = get_byte<1>(word);
    oam[addr + 1] = get_byte<0>(word);
}

inline void OamBus::write_row(const uint8_t row_addr, const uint16_t w0, const uint16_t w1, const uint16_t w2, const uint16_t w3) {
    write_word(row_addr + 0, w0);
    write_word(row_addr + 2, w1);
    write_word(row_addr + 4, w2);
    write_word(row_addr + 6, w3);
}

inline void OamBus::oam_bug_read(const uint8_t row_addr) {
    //  (q) | row - 8  |      |  q0  |  q1  |  q2  |  q3  |  =>  |  f*  |  q1   |  q2   |  q3  |
    //  (r) | row      |      |  r0  |  r1  |  r2  |  r3  |  =>  |  f*  |  q1*  |  q2*  |  q3* |

    const uint16_t q0 = read_word(row_addr - 8 + 0);
    const uint16_t q1 = read_word(row_addr - 8 + 2);
    const uint16_t q2 = read_word(row_addr - 8 + 4);
    const uint16_t q3 = read_word(row_addr - 8 + 6);
    const uint16_t r0 = read_word(row_addr);

    const uint16_t f = q0 | (q2 & r0);

    write_word(row_addr - 8, f);
    write_row(row_addr, f, q1, q2, q3);
}

inline void OamBus::oam_bug_write(const uint8_t row_addr) {
    //  (q) | row - 8  |      |  q0  |  q1  |  q2  |  q3  |  =>  |  q0  |  q1   |  q2   |  q3  |
    //  (r) | row      |      |  r0  |  r1  |  r2  |  r3  |  =>  |  f*  |  q1*  |  q2*  |  q3* |

    const uint16_t q0 = read_word(row_addr - 8);
    const uint16_t q1 = read_word(row_addr - 8 + 2);
    const uint16_t q2 = read_word(row_addr - 8 + 4);
    const uint16_t q3 = read_word(row_addr - 8 + 6);
    const uint16_t r0 = read_word(row_addr);

    const uint16_t f = (r0 & (q0 | q2)) | (q0 & q2);

    write_row(row_addr, f, q1, q2, q3);
}

inline void OamBus::oam_bug_read_write(const uint8_t row_addr) {
    //  (p) | row - 16  |     |  p0  |  p1  |  p2  |  p3  |  =>  |  f*  |  q1*  |  q2*   | q3* |
    //  (q) | row - 8  |      |  q0  |  q1  |  q2  |  q3  |  =>  |  f*  |  q1   |  q2   |  q3  |
    //  (r) | row      |      |  r0  |  r1  |  r2  |  r3  |  =>  |  f*  |  q1*  |  q2*  |  q3* |

    const uint16_t p0 = read_word(row_addr - 16);
    const uint16_t q0 = read_word(row_addr - 8);
    const uint16_t q1 = read_word(row_addr - 8 + 2);
    const uint16_t q2 = read_word(row_addr - 8 + 4);
    const uint16_t q3 = read_word(row_addr - 8 + 6);
    const uint16_t r0 = read_word(row_addr);

    const uint16_t f = (q0 & (p0 | q2 | r0)) | (p0 & q2 & r0);

    write_row(row_addr - 16, f, q1, q2, q3);
    write_word(row_addr - 8, f);
    write_row(row_addr, f, q1, q2, q3);

    if (mod<32>(row_addr) == 0) {
        write_row(row_addr - 32, f, q1, q2, q3);

        if (mod<64>(row_addr) == 0) {
            write_row(0, f, q1, q2, q3);
        }
    }
}

inline void OamBus::oam_bug_read_00() {
    //  (z) | 0x00     |      |  z0  |  z1  |  z2  |  z3  |  =>  |  t0*  |  t1*  |  t2*  |  t3* |
    //  ...
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  t0   |  t1   |  t2   |  t3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t t0 = read_word(target_row_addr);
    const uint16_t t1 = read_word(target_row_addr + 2);
    const uint16_t t2 = read_word(target_row_addr + 4);
    const uint16_t t3 = read_word(target_row_addr + 6);

    write_row(0, t0, t1, t2, t3);
}

inline void OamBus::oam_bug_write_00_0() {
    //  (z) | 0x00     |      |  z0  |  z1  |  z2  |  z3  |  =>  |  t0*  |  t1*  |  t2*  |  t3* |
    //  ...
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  t0   |  t1   |  t2   |  t3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t t0 = read_word(target_row_addr + 0);
    const uint16_t t1 = read_word(target_row_addr + 2);
    const uint16_t t2 = read_word(target_row_addr + 4);
    const uint16_t t3 = read_word(target_row_addr + 6);

    write_row(0, t0, t1, t2, t3);
}

inline void OamBus::oam_bug_write_00_2() {
    //  (z) | 0x00     |      |  z0  |  z1  |  z2  |  z3  |  =>  |  f*  |  g*  |  t2*  |  t3* |
    //  ...
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  t0  |  t1  |  t2   |  t3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t z0 = read_word(0);
    const uint16_t t0 = read_word(target_row_addr + 0);
    const uint16_t t1 = read_word(target_row_addr + 2);
    const uint16_t t2 = read_word(target_row_addr + 4);
    const uint16_t t3 = read_word(target_row_addr + 6);

    const uint16_t dh = data << 8;

    // Deterministic
    //
    // ------------------------------------- raw boolean expressions -----------------------------
    // --- (obtained with Quine–McCluskey algorithm for all the possible inputs from real DMG) ---
    // clang-format off
    //
    // z0 : z[01]t[01] + dt[01] + dz[01]
    // z1 : t[01]t[23] + z[01]t[23] + z[01]t[01]
    // z2 : T
    // z3 : t[23]
    // z4 : t[45]
    // z5 : t[45]
    // z6 : t[67]
    // z7 : t[67]
    //
    // clang-format on
    // --------------------------------------------------------------------------------------------
    //
    // Note: do not use (t1 & 0xFF00) [already written]

    const uint16_t fh = ((z0 & (t0 | dh)) | (t0 & dh)) & 0xFF00;
    const uint16_t fl = ((z0 & (t0 | t1)) | (t0 & t1)) & 0x00FF;
    const uint16_t gh = dh;
    const uint16_t gl = t1 & 0x00FF;

    write_row(0, fh | fl, gh | gl, t2, t3);
}

inline void OamBus::oam_bug_write_00_3() {
    //  (z) | 0x00     |      |  z0  |  z1  |  z2  |  z3  |  =>  |  f*  |  g*  |  t2*  |  t3* |
    //  ...
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  t0  |  t1  |  t2   |  t3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t dl = data;

    const uint16_t z0 = read_word(0);
    const uint16_t t0 = read_word(target_row_addr + 0);
    const uint16_t t1 = read_word(target_row_addr + 2);
    const uint16_t t2 = read_word(target_row_addr + 4);
    const uint16_t t3 = read_word(target_row_addr + 6);

    // Not deterministic (noisy *)
    //
    // ------------------------------------- raw boolean expressions -----------------------------
    // --- (obtained with Quine–McCluskey algorithm for all the possible inputs from real DMG) ---
    // clang-format off
    //
    // z0 : t[01]t[23] + z[01]t[23] + z[01]t[01]
    // z1 : z[01]t[01] + Tt[01] + Tz[01]
    // z2 : t[23]
    // z3*: z[23]z[45]'z[67]'t[01]t[23]'t[45]'t[67]' + z[01]z[23]z[45]'z[67]'t[01]t[45]'t[67]' +
    //      z[23]z[45]'z[67]t[01]t[23]'t[45]'t[67] + z[01]z[23]z[45]'z[67]t[01]t[45]'t[67] +
    //      z[01]'z[23]z[45]z[67]'t[01]t[45]t[67]' + z[01]'z[23]z[45]z[67]t[01]t[45]t[67] +
    //      z[23]z[45]z[67]'t[01]t[23]'t[45]t[67]' + T
    // z4 : t[45]
    // z5 : t[45]
    // z6 : t[67]
    // z7*: t[67] + T'z[01]'z[23]z[67]t[01]'t[23]'t[45] + T'z[01]'z[23]'z[45]'z[67]t[01]'t[23]t[45]' +
    //      T'z[01]'z[23]z[45]'z[67]t[01]t[23]'t[45]'
    //
    // clang-format on
    // --------------------------------------------------------------------------------------------
    //
    // Note: do not use (t1 & 0x00FF) [already written]

    const uint16_t fh = ((z0 & (t0 | t1)) | (t0 & t1)) & 0xFF00;
    const uint16_t fl = ((z0 & (t0 | dl)) | (t0 & dl)) & 0x00FF;
    const uint16_t gh = t1 & 0xFF00;
    const uint16_t gl = dl;

    write_row(0, fh | fl, gh | gl, t2, t3);
}
inline void OamBus::oam_bug_write_00_4() {
    //  (z) | 0x00     |      |  z0  |  z1  |  z2  |  z3  |  =>  |  f*  |  t1* |  g*  |  t3* |
    //  ...
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  t0  |  t1  |  t2  |  t3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t dh = data << 8;
    const uint16_t t2h = mcycle_write.previous_data << 8;

    const uint16_t z0 = read_word(0);
    const uint16_t z1 = read_word(2);
    const uint16_t z2 = read_word(4);
    const uint16_t z3 = read_word(6);
    const uint16_t t0 = read_word(target_row_addr + 0);
    const uint16_t t1 = read_word(target_row_addr + 2);
    const uint16_t t2 = read_word(target_row_addr + 4);
    const uint16_t t3 = read_word(target_row_addr + 6);

    // Deterministic
    //
    // ------------------------------------- raw boolean expressions -----------------------------
    // --- (obtained with Quine–McCluskey algorithm for all the possible inputs from real DMG) ---
    // clang-format off
    //
    // z0 : z[01]t[01] + Tt[01] + Tz[01]
    // z1 : t[01]t[45] + z[01]t[45] + z[01]t[01]
    // z2 : t[23]
    // z3 : t[23]
    // z4 : Tz[67]'t[67] + Tz[45] + Tt[45] + Tz[23]'t[23] + Tt[01] + Tz[67]t[67]' + Tz[23]t[23]'
    // z5 : t[45]
    // z6 : t[67]
    // z7 : t[67]
    //
    // clang-format on
    // --------------------------------------------------------------------------------------------
    //
    // Note: do not use (t2 & 0xFF00) [already written]

    const uint16_t fh = ((z0 & (t0 | dh)) | (t0 & dh)) & 0xFF00;
    const uint16_t fl = ((z0 & (t0 | t2)) | (t0 & t2)) & 0x00FF;
    const uint16_t gh = dh & (t0 | z2 | t2h | (z1 ^ t1) | (z3 ^ t3)) & 0xFF00;
    const uint16_t gl = t2 & 0x00FF;

    write_row(0, fh | fl, t1, gh | gl, t3);
}

inline void OamBus::oam_bug_write_00_5() {
    //  (z) | 0x00     |      |  z0  |  z1  |  z2  |  z3  |  =>  |  f*  |  t1* |  g*  |  t3* |
    //  ...
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  t0  |  t1  |  t2  |  t3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t dl = data;

    const uint16_t z0 = read_word(0);
    const uint16_t t0 = read_word(target_row_addr + 0);
    const uint16_t t1 = read_word(target_row_addr + 2);
    const uint16_t t2 = read_word(target_row_addr + 4);
    const uint16_t t3 = read_word(target_row_addr + 6);

    // Not deterministic (noisy *)
    //
    // ------------------------------------- raw boolean expressions -----------------------------
    // --- (obtained with Quine–McCluskey algorithm for all the possible inputs from real DMG) ---
    // clang-format off
    //
    // z0 : t[01]t[45] + z[01]t[45] + z[01]t[01]
    // z1 : z[01]t[01] + Tt[01] + Tz[01]
    // z2 : t[23]
    // z3 : t[23]
    // z4 : t[45]
    // z5 : T
    // z6 : t[67]
    // z7*: t[67] + T'z[01]z[23]z[45]'z[67]t[01]'t[23]t[45] + T'z[01]'z[23]'z[67]t[01]'t[23]'t[45]' +
    //      T'z[01]'z[23]'z[45]z[67]t[01]'t[23]' + T'z[23]z[45]z[67]t[23]t[45]' + T'z[23]'z[45]z[67]t[23]'t[45]' +
    //      T'z[01]z[23]'z[67]t[01]t[23]'t[45]' + T'z[01]'z[23]z[67]t[01]'t[23]t[45]' +
    //      Tz[01]z[23]z[45]'z[67]t[01]t[23]t[45]' + T'z[01]z[23]'z[45]z[67]t[01]'t[23]t[45] +
    //      Tz[01]'z[23]z[45]'z[67]t[01]t[23]t[45] + Tz[01]z[23]z[45]z[67]t[01]t[23]'t[45]'
    //
    // clang-format on
    // --------------------------------------------------------------------------------------------
    //
    // Note: do not use (t2 & 0x00FF) [already written]

    const uint16_t fh = ((z0 & (t0 | t2)) | (t0 & t2)) & 0xFF00;
    const uint16_t fl = ((z0 & (t0 | dl)) | (t0 & dl)) & 0x00FF;
    const uint16_t gh = t2 & 0xFF00;
    const uint16_t gl = dl;

    write_row(0, fh | fl, t1, gh | gl, t3);
}

inline void OamBus::oam_bug_write_00_6() {
    //  (z) | 0x00     |      |  z0  |  z1  |  z2  |  z3  |  =>  |  f*  |  t1* |  t2* |  g*  |
    //  ...
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  t0  |  t1  |  t2  |  t3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t dh = data << 8;

    const uint16_t z0 = read_word(0);
    const uint16_t z3 = read_word(6);
    const uint16_t t0 = read_word(target_row_addr + 0);
    const uint16_t t1 = read_word(target_row_addr + 2);
    const uint16_t t2 = read_word(target_row_addr + 4);
    const uint16_t t3 = read_word(target_row_addr + 6);

    // Not deterministic (noisy *)
    //
    // ------------------------------------- raw boolean expressions -----------------------------
    // --- (obtained with Quine–McCluskey algorithm for all the possible inputs from real DMG) ---
    // clang-format off
    //
    // z0 : z[01]t[01] + Tt[01] + Tz[01]
    // z1 : t[01]t[67] + z[01]t[67] + z[01]t[01]
    // z2 : t[23]
    // z3 : t[23]
    // z4 : t[45]
    // z5 : t[45]
    // z6*: z[23]'z[45]'z[67]t[23]'t[45]'t[67]' + z[67]t[01] + z[23]'z[45]z[67]t[23]'t[45]t[67]' +
    //      z[01]'z[23]z[45]'z[67]t[23]t[45]'t[67]' + T + z[23]z[45]z[67]t[23]t[45]t[67]'
    // z7 : t[67]
    //
    // clang-format on
    // --------------------------------------------------------------------------------------------
    //
    // Note: do not use (t3 & 0xFF00) [already written]

    const uint16_t fh = ((z0 & (t0 | dh)) | (t0 & dh)) & 0xFF00;
    const uint16_t fl = ((z0 & (t0 | t3)) | (t0 & t3)) & 0x00FF;
    const uint16_t gh = (dh | (t0 & z3)) & 0xFF00;
    const uint16_t gl = t3 & 0x00FF;

    write_row(0, fh | fl, t1, t2, gh | gl);
}

inline void OamBus::oam_bug_write_00_7() {
    //  (z) | 0x00     |      |  z0  |  z1  |  z2  |  z3  |  =>  |  f*  |  t1* |  t2* |  g*  |
    //  ...
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  t0  |  t1  |  t2  |  t3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t dl = data;

    const uint16_t z0 = read_word(0);
    const uint16_t z3 = read_word(6);
    const uint16_t t0 = read_word(target_row_addr + 0);
    const uint16_t t1 = read_word(target_row_addr + 2);
    const uint16_t t2 = read_word(target_row_addr + 4);
    const uint16_t t3 = read_word(target_row_addr + 6);

    // Deterministic
    //
    // ------------------------------------- raw boolean expressions -----------------------------
    // --- (obtained with Quine–McCluskey algorithm for all the possible inputs from real DMG) ---
    // clang-format off
    //
    // z0 : t[01]t[67] + z[01]t[67] + z[01]t[01]
    // z1 : z[01]t[01] + Tt[01] + Tz[01]
    // z2 : t[23]
    // z3 : t[23]
    // z4 : t[45]
    // z5 : t[45]
    // z6 : t[67]
    // z7 : z[67] + T
    //
    // clang-format on
    // --------------------------------------------------------------------------------------------
    //
    // Note: do not use (t3 & 0x00FF) [already written]

    const uint16_t fh = ((z0 & (t0 | t3)) | (t0 & t3)) & 0xFF00;
    const uint16_t fl = ((z0 & (t0 | dl)) | (t0 & dl)) & 0x00FF;
    const uint16_t gh = t3 & 0xFF00;
    const uint16_t gl = (z3 | dl) & 0x00FF;

    write_row(0, fh | fl, t1, t2, gh | gl);
}

inline void OamBus::oam_bug_read_9c_6() {
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  n0* |  n1* |  n2* |  f*  |
    //  ...
    //  (a) | 0x98     |      |  n0  |  n1  |  n2  |  n3  |  =>  |  n0  |  n1  |  n2  |  f*  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t t0 = read_word(target_row_addr);
    const uint16_t n0 = read_word(0x98);
    const uint16_t n1 = read_word(0x98 + 2);
    const uint16_t n2 = read_word(0x98 + 4);
    const uint16_t n3 = read_word(0x98 + 6);

    const uint16_t f = n3 | (t0 & n2);

    write_word(0x98 + 6, f);
    write_row(target_row_addr, n0, n1, n2, f);
}

inline void OamBus::oam_bug_read_9c_2() {
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  n0* |  f*  |  n2* |  n3* |
    //  ...
    //  (a) | 0x98     |      |  n0  |  n1  |  n2  |  n3  |  =>  |  n0  |  f*  |  n2  |  n3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t t1 = read_word(target_row_addr + 2);
    const uint16_t n0 = read_word(0x98);
    const uint16_t n1 = read_word(0x98 + 2);
    const uint16_t n2 = read_word(0x98 + 4);
    const uint16_t n3 = read_word(0x98 + 6);

    const uint16_t f = (t1 & (n1 | n2)) | (n1 & n2);

    write_word(0x98 + 2, f);
    write_row(target_row_addr, n0, f, n2, n3);
}

inline void OamBus::oam_bug_read_9c_0_4() {
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  f*  |  n1* |  n2* |  n3* |
    //  ...
    //  (a) | 0x98     |      |  n0  |  n1  |  n2  |  n3  |  =>  |  f*  |  n1  |  n2  |  n3  |


    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t t0 = read_word(target_row_addr);
    const uint16_t n0 = read_word(0x98);
    const uint16_t n1 = read_word(0x98 + 2);
    const uint16_t n2 = read_word(0x98 + 4);
    const uint16_t n3 = read_word(0x98 + 6);

    const uint16_t f = (t0 & (n0 | n2)) | (n0 & n2);

    write_word(0x98, f);
    write_row(target_row_addr, f, n1, n2, n3);
}

inline void OamBus::oam_bug_write_9c_0() {
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  f*  |  n1* |  n2* |  n3* |
    //  ...
    //  (a) | 0x98     |      |  n0  |  n1  |  n2  |  n3  |  =>  |  n0  |  n1  |  n2  |  n3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t t0 = read_word(target_row_addr);
    const uint16_t n0 = read_word(0x98);
    const uint16_t n1 = read_word(0x98 + 2);
    const uint16_t n2 = read_word(0x98 + 4);
    const uint16_t n3 = read_word(0x98 + 6);

    const uint16_t f = (t0 & (n0 | n2)) | (n0 & n2);

    write_row(target_row_addr, f, n1, n2, n3);
}

inline void OamBus::oam_bug_write_9c_2() {
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  n0* |  f*  |  n2* |  n3* |
    //  ...
    //  (a) | 0x98     |      |  n0  |  n1  |  n2  |  n3  |  =>  |  n0  |  n1  |  n2  |  n3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t n0 = read_word(0x98);
    const uint16_t n1 = read_word(0x98 + 2);
    const uint16_t n2 = read_word(0x98 + 4);
    const uint16_t n3 = read_word(0x98 + 6);

    const uint16_t f = n1 & n2;

    write_row(target_row_addr, n0, f, n2, n3);
}

inline void OamBus::oam_bug_write_9c_4() {
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  n0* |  n1* |  n2* |  n3* |
    //  ...
    //  (a) | 0x98     |      |  n0  |  n1  |  n2  |  n3  |  =>  |  n0  |  n1  |  n2  |  n3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t n0 = read_word(0x98);
    const uint16_t n1 = read_word(0x98 + 2);
    const uint16_t n2 = read_word(0x98 + 4);
    const uint16_t n3 = read_word(0x98 + 6);

    write_row(target_row_addr, n0, n1, n2, n3);
}

inline void OamBus::oam_bug_write_9c_6() {
    //  (t) | target   |      |  t0  |  t1  |  t2  |  t3  |  =>  |  n0* |  n1* |  f*  |  g*  |
    //  ...
    //  (a) | 0x98     |      |  n0  |  n1  |  n2  |  n3  |  =>  |  n0  |  n1  |  n2  |  n3  |

    const uint8_t target_row_addr = discard_bits<3>(address);

    const uint16_t t2 = read_word(target_row_addr + 4);
    const uint16_t t3 = read_word(target_row_addr + 6);

    const uint16_t n0 = read_word(0x98);
    const uint16_t n1 = read_word(0x98 + 2);
    const uint16_t n2 = read_word(0x98 + 4);
    const uint16_t n3 = read_word(0x98 + 6);

    const uint16_t f = n2 & (t2 | n3);
    const uint16_t g = (n2 & (t3 | n3)) | (t3 & n3);

    write_row(target_row_addr, n0, n1, f, g);
}