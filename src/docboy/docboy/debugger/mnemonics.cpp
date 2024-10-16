#include "docboy/debugger/mnemonics.h"

#include <map>

#include "docboy/common/specs.h"

#include "utils/bits.h"
#include "utils/formatters.h"

struct InstructionInfo {
    const char* mnemonic;
    uint8_t length;
    std::pair<uint8_t, uint8_t> duration;
};

constexpr InstructionInfo INSTRUCTIONS[256] = {
    /* 00 */ {"NOP", 1, {1, 1}},
    /* 01 */ {"LD BC,$%04X", 3, {3, 3}},
    /* 02 */ {"LD (BC),A", 1, {2, 2}},
    /* 03 */ {"INC BC", 1, {2, 2}},
    /* 04 */ {"INC B", 1, {1, 1}},
    /* 05 */ {"DEC B", 1, {1, 1}},
    /* 06 */ {"LD B,$%02X", 2, {2, 2}},
    /* 07 */ {"RLCA", 1, {1, 1}},
    /* 08 */ {"LD ($%04X),SP", 3, {5, 5}},
    /* 09 */ {"ADD HL,BC", 1, {2, 2}},
    /* 0A */ {"LD A,(BC)", 1, {2, 2}},
    /* 0B */ {"DEC BC", 1, {2, 2}},
    /* 0C */ {"INC C", 1, {1, 1}},
    /* 0D */ {"DEC C", 1, {1, 1}},
    /* 0E */ {"LD C,$%02X", 2, {2, 2}},
    /* 0F */ {"RRCA", 1, {1, 1}},
    //	/* 10 */ { "STOP", 2, {1, 1} },
    /* 10 */ {"STOP", 1, {1, 1}}, // TODO: 1 or 2 byte long?
    /* 11 */ {"LD DE,$%04X", 3, {3, 3}},
    /* 12 */ {"LD (DE),A", 1, {2, 2}},
    /* 13 */ {"INC DE", 1, {2, 2}},
    /* 14 */ {"INC D", 1, {1, 1}},
    /* 15 */ {"DEC D", 1, {1, 1}},
    /* 16 */ {"LD D,$%02X", 2, {2, 2}},
    /* 17 */ {"RLA", 1, {1, 1}},
    /* 18 */ {"JR $%04X  [%+d]", 2, {3, 3}},
    /* 19 */ {"ADD HL,DE", 1, {2, 2}},
    /* 1A */ {"LD A,(DE)", 1, {2, 2}},
    /* 1B */ {"DEC DE", 1, {2, 2}},
    /* 1C */ {"INC E", 1, {1, 1}},
    /* 1D */ {"DEC E", 1, {1, 1}},
    /* 1E */ {"LD E,$%02X", 2, {2, 2}},
    /* 1F */ {"RRA", 1, {1, 1}},
    /* 20 */ {"JR NZ,$%04X  [%+d]", 2, {2, 3}},
    /* 21 */ {"LD HL,$%04X", 3, {3, 3}},
    /* 22 */ {"LD (HL+),A", 1, {2, 2}},
    /* 23 */ {"INC HL", 1, {2, 2}},
    /* 24 */ {"INC H", 1, {1, 1}},
    /* 25 */ {"DEC H", 1, {1, 1}},
    /* 26 */ {"LD H,$%02X", 2, {2, 2}},
    /* 27 */ {"DAA", 1, {1, 1}},
    /* 28 */ {"JR Z,$%04X  [%+d]", 2, {2, 3}},
    /* 29 */ {"ADD HL,HL", 1, {2, 2}},
    /* 2A */ {"LD A,(HL+)", 1, {2, 2}},
    /* 2B */ {"DEC HL", 1, {2, 2}},
    /* 2C */ {"INC L", 1, {1, 1}},
    /* 2D */ {"DEC L", 1, {1, 1}},
    /* 2E */ {"LD L,$%02X", 2, {2, 2}},
    /* 2F */ {"CPL", 1, {1, 1}},
    /* 30 */ {"JR NC,$%04X  [%+d]", 2, {2, 3}},
    /* 31 */ {"LD SP,$%04X", 3, {3, 3}},
    /* 32 */ {"LD (HL-),A", 1, {2, 2}},
    /* 33 */ {"INC SP", 1, {2, 2}},
    /* 34 */ {"INC (HL)", 1, {3, 3}},
    /* 35 */ {"DEC (HL)", 1, {3, 3}},
    /* 36 */ {"LD (HL),$%02X", 2, {3, 3}},
    /* 37 */ {"SCF", 1, {1, 1}},
    /* 38 */ {"JR C,$%04X  [%+d]", 2, {2, 3}},
    /* 39 */ {"ADD HL,SP", 1, {2, 2}},
    /* 3A */ {"LD A,(HL-)", 1, {2, 2}},
    /* 3B */ {"DEC SP", 1, {2, 2}},
    /* 3C */ {"INC A", 1, {1, 1}},
    /* 3D */ {"DEC A", 1, {1, 1}},
    /* 3E */ {"LD A,$%02X", 2, {2, 2}},
    /* 3F */ {"CCF", 1, {1, 1}},
    /* 40 */ {"LD B,B", 1, {1, 1}},
    /* 41 */ {"LD B,C", 1, {1, 1}},
    /* 42 */ {"LD B,D", 1, {1, 1}},
    /* 43 */ {"LD B,E", 1, {1, 1}},
    /* 44 */ {"LD B,H", 1, {1, 1}},
    /* 45 */ {"LD B,L", 1, {1, 1}},
    /* 46 */ {"LD B,(HL)", 1, {2, 2}},
    /* 47 */ {"LD B,A", 1, {1, 1}},
    /* 48 */ {"LD C,B", 1, {1, 1}},
    /* 49 */ {"LD C,C", 1, {1, 1}},
    /* 4A */ {"LD C,D", 1, {1, 1}},
    /* 4B */ {"LD C,E", 1, {1, 1}},
    /* 4C */ {"LD C,H", 1, {1, 1}},
    /* 4D */ {"LD C,L", 1, {1, 1}},
    /* 4E */ {"LD C,(HL)", 1, {2, 2}},
    /* 4F */ {"LD C,A", 1, {1, 1}},
    /* 50 */ {"LD D,B", 1, {1, 1}},
    /* 51 */ {"LD D,C", 1, {1, 1}},
    /* 52 */ {"LD D,D", 1, {1, 1}},
    /* 53 */ {"LD D,E", 1, {1, 1}},
    /* 54 */ {"LD D,H", 1, {1, 1}},
    /* 55 */ {"LD D,L", 1, {1, 1}},
    /* 56 */ {"LD D,(HL)", 1, {2, 2}},
    /* 57 */ {"LD D,A", 1, {1, 1}},
    /* 58 */ {"LD E,B", 1, {1, 1}},
    /* 59 */ {"LD E,C", 1, {1, 1}},
    /* 5A */ {"LD E,D", 1, {1, 1}},
    /* 5B */ {"LD E,E", 1, {1, 1}},
    /* 5C */ {"LD E,H", 1, {1, 1}},
    /* 5D */ {"LD E,L", 1, {1, 1}},
    /* 5E */ {"LD E,(HL)", 1, {2, 2}},
    /* 5F */ {"LD E,A", 1, {1, 1}},
    /* 60 */ {"LD H,B", 1, {1, 1}},
    /* 61 */ {"LD H,C", 1, {1, 1}},
    /* 62 */ {"LD H,D", 1, {1, 1}},
    /* 63 */ {"LD H,E", 1, {1, 1}},
    /* 64 */ {"LD H,H", 1, {1, 1}},
    /* 65 */ {"LD H,L", 1, {1, 1}},
    /* 66 */ {"LD H,(HL)", 1, {2, 2}},
    /* 67 */ {"LD H,A", 1, {1, 1}},
    /* 68 */ {"LD L,B", 1, {1, 1}},
    /* 69 */ {"LD L,C", 1, {1, 1}},
    /* 6A */ {"LD L,D", 1, {1, 1}},
    /* 6B */ {"LD L,E", 1, {1, 1}},
    /* 6C */ {"LD L,H", 1, {1, 1}},
    /* 6D */ {"LD L,L", 1, {1, 1}},
    /* 6E */ {"LD L,(HL)", 1, {2, 2}},
    /* 6F */ {"LD L,A", 1, {1, 1}},
    /* 70 */ {"LD (HL),B", 1, {2, 2}},
    /* 71 */ {"LD (HL),C", 1, {2, 2}},
    /* 72 */ {"LD (HL),D", 1, {2, 2}},
    /* 73 */ {"LD (HL),E", 1, {2, 2}},
    /* 74 */ {"LD (HL),H", 1, {2, 2}},
    /* 75 */ {"LD (HL),L", 1, {2, 2}},
    /* 76 */ {"HALT", 1, {1, 1}},
    /* 77 */ {"LD (HL),A", 1, {2, 2}},
    /* 78 */ {"LD A,B", 1, {1, 1}},
    /* 79 */ {"LD A,C", 1, {1, 1}},
    /* 7A */ {"LD A,D", 1, {1, 1}},
    /* 7B */ {"LD A,E", 1, {1, 1}},
    /* 7C */ {"LD A,H", 1, {1, 1}},
    /* 7D */ {"LD A,L", 1, {1, 1}},
    /* 7E */ {"LD A,(HL)", 1, {2, 2}},
    /* 7F */ {"LD A,A", 1, {1, 1}},
    /* 80 */ {"ADD A,B", 1, {1, 1}},
    /* 81 */ {"ADD A,C", 1, {1, 1}},
    /* 82 */ {"ADD A,D", 1, {1, 1}},
    /* 83 */ {"ADD A,E", 1, {1, 1}},
    /* 84 */ {"ADD A,H", 1, {1, 1}},
    /* 85 */ {"ADD A,L", 1, {1, 1}},
    /* 86 */ {"ADD A,(HL)", 1, {2, 2}},
    /* 87 */ {"ADD A,A", 1, {1, 1}},
    /* 88 */ {"ADC A,B", 1, {1, 1}},
    /* 89 */ {"ADC A,C", 1, {1, 1}},
    /* 8A */ {"ADC A,D", 1, {1, 1}},
    /* 8B */ {"ADC A,E", 1, {1, 1}},
    /* 8C */ {"ADC A,H", 1, {1, 1}},
    /* 8D */ {"ADC A,L", 1, {1, 1}},
    /* 8E */ {"ADC A,(HL)", 1, {2, 2}},
    /* 8F */ {"ADC A,A", 1, {1, 1}},
    /* 90 */ {"SUB B", 1, {1, 1}},
    /* 91 */ {"SUB C", 1, {1, 1}},
    /* 92 */ {"SUB D", 1, {1, 1}},
    /* 93 */ {"SUB E", 1, {1, 1}},
    /* 94 */ {"SUB H", 1, {1, 1}},
    /* 95 */ {"SUB L", 1, {1, 1}},
    /* 96 */ {"SUB (HL)", 1, {2, 2}},
    /* 97 */ {"SUB A", 1, {1, 1}},
    /* 98 */ {"SBC A,B", 1, {1, 1}},
    /* 99 */ {"SBC A,C", 1, {1, 1}},
    /* 9A */ {"SBC A,D", 1, {1, 1}},
    /* 9B */ {"SBC A,E", 1, {1, 1}},
    /* 9C */ {"SBC A,H", 1, {1, 1}},
    /* 9D */ {"SBC A,L", 1, {1, 1}},
    /* 9E */ {"SBC A,(HL)", 1, {2, 2}},
    /* 9F */ {"SBC A,A", 1, {1, 1}},
    /* A0 */ {"AND B", 1, {1, 1}},
    /* A1 */ {"AND C", 1, {1, 1}},
    /* A2 */ {"AND D", 1, {1, 1}},
    /* A3 */ {"AND E", 1, {1, 1}},
    /* A4 */ {"AND H", 1, {1, 1}},
    /* A5 */ {"AND L", 1, {1, 1}},
    /* A6 */ {"AND (HL)", 1, {2, 2}},
    /* A7 */ {"AND A", 1, {1, 1}},
    /* A8 */ {"XOR B", 1, {1, 1}},
    /* A9 */ {"XOR C", 1, {1, 1}},
    /* AA */ {"XOR D", 1, {1, 1}},
    /* AB */ {"XOR E", 1, {1, 1}},
    /* AC */ {"XOR H", 1, {1, 1}},
    /* AD */ {"XOR L", 1, {1, 1}},
    /* AE */ {"XOR (HL)", 1, {2, 2}},
    /* AF */ {"XOR A", 1, {1, 1}},
    /* B0 */ {"OR B", 1, {1, 1}},
    /* B1 */ {"OR C", 1, {1, 1}},
    /* B2 */ {"OR D", 1, {1, 1}},
    /* B3 */ {"OR E", 1, {1, 1}},
    /* B4 */ {"OR H", 1, {1, 1}},
    /* B5 */ {"OR L", 1, {1, 1}},
    /* B6 */ {"OR (HL)", 1, {2, 2}},
    /* B7 */ {"OR A", 1, {1, 1}},
    /* B8 */ {"CP B", 1, {1, 1}},
    /* B9 */ {"CP C", 1, {1, 1}},
    /* BA */ {"CP D", 1, {1, 1}},
    /* BB */ {"CP E", 1, {1, 1}},
    /* BC */ {"CP H", 1, {1, 1}},
    /* BD */ {"CP L", 1, {1, 1}},
    /* BE */ {"CP (HL)", 1, {2, 2}},
    /* BF */ {"CP A", 1, {1, 1}},
    /* C0 */ {"RET NZ", 1, {2, 5}},
    /* C1 */ {"POP BC", 1, {3, 3}},
    /* C2 */ {"JP NZ,$%04X", 3, {3, 4}},
    /* C3 */ {"JP $%04X", 3, {4, 4}},
    /* C4 */ {"CALL NZ,$%04X", 3, {3, 6}},
    /* C5 */ {"PUSH BC", 1, {4, 4}},
    /* C6 */ {"ADD A,$%02X", 2, {2, 2}},
    /* C7 */ {"RST ", 1, {4, 4}},
    /* C8 */ {"RET Z", 1, {2, 5}},
    /* C9 */ {"RET", 1, {4, 4}},
    /* CA */ {"JP Z,$%04X", 3, {3, 4}},
    /* CB */ {"CB prefix", 2, {0, 0}},
    /* CC */ {"CALL Z,$%04X", 3, {3, 6}},
    /* CD */ {"CALL $%04X", 3, {6, 6}},
    /* CE */ {"ADC A,$%02X", 2, {2, 2}},
    /* CF */ {"RST $08", 1, {4, 4}},
    /* D0 */ {"RET NC", 1, {2, 5}},
    /* D1 */ {"POP DE", 1, {3, 3}},
    /* D2 */ {"JP NC,$%04X", 3, {3, 4}},
    /* D3 */ {"[UNUSED]", 1, {0, 0}},
    /* D4 */ {"CALL NC,$%04X", 3, {3, 6}},
    /* D5 */ {"PUSH DE", 1, {4, 4}},
    /* D6 */ {"SUB $%02X", 2, {2, 2}},
    /* D7 */ {"RST $10", 1, {4, 4}},
    /* D8 */ {"RET C", 1, {2, 5}},
    /* D9 */ {"RETI", 1, {4, 4}},
    /* DA */ {"JP C,$%04X", 3, {3, 4}},
    /* DB */ {"[UNUSED]", 1, {0, 0}},
    /* DC */ {"CALL C,$%04X", 3, {3, 6}},
    /* DD */ {"[UNUSED]", 1, {0, 0}},
    /* DE */ {"SBC A,$%02X", 2, {2, 2}},
    /* DF */ {"RST $18", 1, {4, 4}},
    /* E0 */ {"LD ($FF00+$%02X),A  [%s]", 2, {3, 3}},
    /* E1 */ {"POP HL", 1, {3, 3}},
    /* E2 */ {"LD ($FF00+C),A", 1, {2, 2}},
    /* E3 */ {"[UNUSED]", 1, {0, 0}},
    /* E4 */ {"[UNUSED]", 1, {0, 0}},
    /* E5 */ {"PUSH HL", 1, {4, 4}},
    /* E6 */ {"AND $%02X", 2, {2, 2}},
    /* E7 */ {"RST $20", 1, {4, 4}},
    /* E8 */ {"ADD SP,%+d", 2, {4, 4}},
    /* E9 */ {"JP (HL)", 1, {1, 1}},
    /* EA */ {"LD ($%04X),A", 3, {4, 4}},
    /* EB */ {"[UNUSED]", 1, {0, 0}},
    /* EC */ {"[UNUSED]", 1, {0, 0}},
    /* ED */ {"[UNUSED]", 1, {0, 0}},
    /* EE */ {"XOR $%02X", 2, {2, 2}},
    /* EF */ {"RST $28", 1, {4, 4}},
    /* F0 */ {"LD A,($FF00+$%02X)  [%s]", 2, {3, 3}},
    /* F1 */ {"POP AF", 1, {3, 3}},
    /* F2 */ {"LD A,($FF00+C)", 1, {2, 2}},
    /* F3 */ {"DI", 1, {1, 1}},
    /* F4 */ {"[UNUSED]", 1, {0, 0}},
    /* F5 */ {"PUSH AF", 1, {4, 4}},
    /* F6 */ {"OR $%02X", 2, {2, 2}},
    /* F7 */ {"RST $30", 1, {4, 4}},
    /* F8 */ {"LD HL,(SP%+d)", 2, {3, 3}},
    /* F9 */ {"LD SP,HL", 1, {2, 2}},
    /* FA */ {"LD A,($%04X)", 3, {4, 4}},
    /* FB */ {"EI", 1, {1, 1}},
    /* FC */ {"[UNUSED]", 1, {0, 0}},
    /* FD */ {"[UNUSED]", 1, {0, 0}},
    /* FE */ {"CP $%02X", 2, {2, 2}},
    /* FF */ {"RST $38", 1, {4, 4}},
};

constexpr InstructionInfo INSTRUCTIONS_CB[256] = {
    /* 00 */ {"RLC B", 2, {2, 2}},
    /* 01 */ {"RLC C", 2, {2, 2}},
    /* 02 */ {"RLC D", 2, {2, 2}},
    /* 03 */ {"RLC E", 2, {2, 2}},
    /* 04 */ {"RLC H", 2, {2, 2}},
    /* 05 */ {"RLC L", 2, {2, 2}},
    /* 06 */ {"RLC (HL)", 2, {4, 4}},
    /* 07 */ {"RLC A", 2, {2, 2}},
    /* 08 */ {"RRC B", 2, {2, 2}},
    /* 09 */ {"RRC C", 2, {2, 2}},
    /* 0A */ {"RRC D", 2, {2, 2}},
    /* 0B */ {"RRC E", 2, {2, 2}},
    /* 0C */ {"RRC H", 2, {2, 2}},
    /* 0D */ {"RRC L", 2, {2, 2}},
    /* 0E */ {"RRC (HL)", 2, {4, 4}},
    /* 0F */ {"RRC A", 2, {2, 2}},
    /* 10 */ {"RL B", 2, {2, 2}},
    /* 11 */ {"RL C", 2, {2, 2}},
    /* 12 */ {"RL D", 2, {2, 2}},
    /* 13 */ {"RL E", 2, {2, 2}},
    /* 14 */ {"RL H", 2, {2, 2}},
    /* 15 */ {"RL L ", 2, {2, 2}},
    /* 16 */ {"RL (HL)", 2, {4, 4}},
    /* 17 */ {"RL A", 2, {2, 2}},
    /* 18 */ {"RR B", 2, {2, 2}},
    /* 19 */ {"RR C", 2, {2, 2}},
    /* 1A */ {"RR D", 2, {2, 2}},
    /* 1B */ {"RR E", 2, {2, 2}},
    /* 1C */ {"RR H", 2, {2, 2}},
    /* 1D */ {"RR L", 2, {2, 2}},
    /* 1E */ {"RR (HL)", 2, {4, 4}},
    /* 1F */ {"RR A", 2, {2, 2}},
    /* 20 */ {"SLA B", 2, {2, 2}},
    /* 21 */ {"SLA C", 2, {2, 2}},
    /* 22 */ {"SLA D", 2, {2, 2}},
    /* 23 */ {"SLA E", 2, {2, 2}},
    /* 24 */ {"SLA H", 2, {2, 2}},
    /* 25 */ {"SLA L", 2, {2, 2}},
    /* 26 */ {"SLA (HL)", 2, {4, 4}},
    /* 27 */ {"SLA A", 2, {2, 2}},
    /* 28 */ {"SRA B", 2, {2, 2}},
    /* 29 */ {"SRA C", 2, {2, 2}},
    /* 2A */ {"SRA D", 2, {2, 2}},
    /* 2B */ {"SRA E", 2, {2, 2}},
    /* 2C */ {"SRA H", 2, {2, 2}},
    /* 2D */ {"SRA L", 2, {2, 2}},
    /* 2E */ {"SRA (HL)", 2, {4, 4}},
    /* 2F */ {"SRA A", 2, {2, 2}},
    /* 30 */ {"SWAP B", 2, {2, 2}},
    /* 31 */ {"SWAP C", 2, {2, 2}},
    /* 32 */ {"SWAP D", 2, {2, 2}},
    /* 33 */ {"SWAP E", 2, {2, 2}},
    /* 34 */ {"SWAP H", 2, {2, 2}},
    /* 35 */ {"SWAP L", 2, {2, 2}},
    /* 36 */ {"SWAP (HL)", 2, {4, 4}},
    /* 37 */ {"SWAP A", 2, {2, 2}},
    /* 38 */ {"SRL B", 2, {2, 2}},
    /* 39 */ {"SRL C", 2, {2, 2}},
    /* 3A */ {"SRL D", 2, {2, 2}},
    /* 3B */ {"SRL E", 2, {2, 2}},
    /* 3C */ {"SRL H", 2, {2, 2}},
    /* 3D */ {"SRL L", 2, {2, 2}},
    /* 3E */ {"SRL (HL)", 2, {4, 4}},
    /* 3F */ {"SRL A", 2, {2, 2}},
    /* 40 */ {"BIT 0 B", 2, {2, 2}},
    /* 41 */ {"BIT 0 C", 2, {2, 2}},
    /* 42 */ {"BIT 0 D", 2, {2, 2}},
    /* 43 */ {"BIT 0 E", 2, {2, 2}},
    /* 44 */ {"BIT 0 H", 2, {2, 2}},
    /* 45 */ {"BIT 0 L", 2, {2, 2}},
    /* 46 */ {"BIT 0 (HL)", 2, {3, 3}},
    /* 47 */ {"BIT 0 A", 2, {2, 2}},
    /* 48 */ {"BIT 1 B", 2, {2, 2}},
    /* 49 */ {"BIT 1 C", 2, {2, 2}},
    /* 4A */ {"BIT 1 D", 2, {2, 2}},
    /* 4B */ {"BIT 1 E", 2, {2, 2}},
    /* 4C */ {"BIT 1 H", 2, {2, 2}},
    /* 4D */ {"BIT 1 L", 2, {2, 2}},
    /* 4E */ {"BIT 1 (HL)", 2, {3, 3}},
    /* 4F */ {"BIT 1 A", 2, {2, 2}},
    /* 50 */ {"BIT 2 B", 2, {2, 2}},
    /* 51 */ {"BIT 2 C", 2, {2, 2}},
    /* 52 */ {"BIT 2 D", 2, {2, 2}},
    /* 53 */ {"BIT 2 E", 2, {2, 2}},
    /* 54 */ {"BIT 2 H", 2, {2, 2}},
    /* 55 */ {"BIT 2 L", 2, {2, 2}},
    /* 56 */ {"BIT 2 (HL)", 2, {3, 3}},
    /* 57 */ {"BIT 2 A", 2, {2, 2}},
    /* 58 */ {"BIT 3 B", 2, {2, 2}},
    /* 59 */ {"BIT 3 C", 2, {2, 2}},
    /* 5A */ {"BIT 3 D", 2, {2, 2}},
    /* 5B */ {"BIT 3 E", 2, {2, 2}},
    /* 5C */ {"BIT 3 H", 2, {2, 2}},
    /* 5D */ {"BIT 3 L", 2, {2, 2}},
    /* 5E */ {"BIT 3 (HL)", 2, {3, 3}},
    /* 5F */ {"BIT 3 A", 2, {2, 2}},
    /* 60 */ {"BIT 4 B", 2, {2, 2}},
    /* 61 */ {"BIT 4 C", 2, {2, 2}},
    /* 62 */ {"BIT 4 D", 2, {2, 2}},
    /* 63 */ {"BIT 4 E", 2, {2, 2}},
    /* 64 */ {"BIT 4 H", 2, {2, 2}},
    /* 65 */ {"BIT 4 L", 2, {2, 2}},
    /* 66 */ {"BIT 4 (HL)", 2, {3, 3}},
    /* 67 */ {"BIT 4 A", 2, {2, 2}},
    /* 68 */ {"BIT 5 B", 2, {2, 2}},
    /* 69 */ {"BIT 5 C", 2, {2, 2}},
    /* 6A */ {"BIT 5 D", 2, {2, 2}},
    /* 6B */ {"BIT 5 E", 2, {2, 2}},
    /* 6C */ {"BIT 5 H", 2, {2, 2}},
    /* 6D */ {"BIT 5 L", 2, {2, 2}},
    /* 6E */ {"BIT 5 (HL)", 2, {3, 3}},
    /* 6F */ {"BIT 5 A", 2, {2, 2}},
    /* 70 */ {"BIT 6 B", 2, {2, 2}},
    /* 71 */ {"BIT 6 C", 2, {2, 2}},
    /* 72 */ {"BIT 6 D", 2, {2, 2}},
    /* 73 */ {"BIT 6 E", 2, {2, 2}},
    /* 74 */ {"BIT 6 H", 2, {2, 2}},
    /* 75 */ {"BIT 6 L", 2, {2, 2}},
    /* 76 */ {"BIT 6 (HL)", 2, {3, 3}},
    /* 77 */ {"BIT 6 A", 2, {2, 2}},
    /* 78 */ {"BIT 7 B", 2, {2, 2}},
    /* 79 */ {"BIT 7 C", 2, {2, 2}},
    /* 7A */ {"BIT 7 D", 2, {2, 2}},
    /* 7B */ {"BIT 7 E", 2, {2, 2}},
    /* 7C */ {"BIT 7 H", 2, {2, 2}},
    /* 7D */ {"BIT 7 L", 2, {2, 2}},
    /* 7E */ {"BIT 7 (HL)", 2, {3, 3}},
    /* 7F */ {"BIT 7 A", 2, {2, 2}},
    /* 80 */ {"RES 0 B", 2, {2, 2}},
    /* 81 */ {"RES 0 C", 2, {2, 2}},
    /* 82 */ {"RES 0 D", 2, {2, 2}},
    /* 83 */ {"RES 0 E", 2, {2, 2}},
    /* 84 */ {"RES 0 H", 2, {2, 2}},
    /* 85 */ {"RES 0 L", 2, {2, 2}},
    /* 86 */ {"RES 0 (HL)", 2, {4, 4}},
    /* 87 */ {"RES 0 A", 2, {2, 2}},
    /* 88 */ {"RES 1 B", 2, {2, 2}},
    /* 89 */ {"RES 1 C", 2, {2, 2}},
    /* 8A */ {"RES 1 D", 2, {2, 2}},
    /* 8B */ {"RES 1 E", 2, {2, 2}},
    /* 8C */ {"RES 1 H", 2, {2, 2}},
    /* 8D */ {"RES 1 L", 2, {2, 2}},
    /* 8E */ {"RES 1 (HL)", 2, {4, 4}},
    /* 8F */ {"RES 1 A", 2, {2, 2}},
    /* 90 */ {"RES 2 B", 2, {2, 2}},
    /* 91 */ {"RES 2 C", 2, {2, 2}},
    /* 92 */ {"RES 2 D", 2, {2, 2}},
    /* 93 */ {"RES 2 E", 2, {2, 2}},
    /* 94 */ {"RES 2 H", 2, {2, 2}},
    /* 95 */ {"RES 2 L", 2, {2, 2}},
    /* 96 */ {"RES 2 (HL)", 2, {4, 4}},
    /* 97 */ {"RES 2 A", 2, {2, 2}},
    /* 98 */ {"RES 3 B", 2, {2, 2}},
    /* 99 */ {"RES 3 C", 2, {2, 2}},
    /* 9A */ {"RES 3 D", 2, {2, 2}},
    /* 9B */ {"RES 3 E", 2, {2, 2}},
    /* 9C */ {"RES 3 H", 2, {2, 2}},
    /* 9D */ {"RES 3 L", 2, {2, 2}},
    /* 9E */ {"RES 3 (HL)", 2, {4, 4}},
    /* 9F */ {"RES 3 A", 2, {2, 2}},
    /* A0 */ {"RES 4 B", 2, {2, 2}},
    /* A1 */ {"RES 4 C", 2, {2, 2}},
    /* A2 */ {"RES 4 D", 2, {2, 2}},
    /* A3 */ {"RES 4 E", 2, {2, 2}},
    /* A4 */ {"RES 4 H", 2, {2, 2}},
    /* A5 */ {"RES 4 L", 2, {2, 2}},
    /* A6 */ {"RES 4 (HL)", 2, {4, 4}},
    /* A7 */ {"RES 4 A", 2, {2, 2}},
    /* A8 */ {"RES 5 B", 2, {2, 2}},
    /* A9 */ {"RES 5 C", 2, {2, 2}},
    /* AA */ {"RES 5 D", 2, {2, 2}},
    /* AB */ {"RES 5 E", 2, {2, 2}},
    /* AC */ {"RES 5 H", 2, {2, 2}},
    /* AD */ {"RES 5 L", 2, {2, 2}},
    /* AE */ {"RES 5 (HL)", 2, {4, 4}},
    /* AF */ {"RES 5 A", 2, {2, 2}},
    /* B0 */ {"RES 6 B", 2, {2, 2}},
    /* B1 */ {"RES 6 C", 2, {2, 2}},
    /* B2 */ {"RES 6 D", 2, {2, 2}},
    /* B3 */ {"RES 6 E", 2, {2, 2}},
    /* B4 */ {"RES 6 H", 2, {2, 2}},
    /* B5 */ {"RES 6 L", 2, {2, 2}},
    /* B6 */ {"RES 6 (HL)", 2, {4, 4}},
    /* B7 */ {"RES 6 A", 2, {2, 2}},
    /* B8 */ {"RES 7 B", 2, {2, 2}},
    /* B9 */ {"RES 7 C", 2, {2, 2}},
    /* BA */ {"RES 7 D", 2, {2, 2}},
    /* BB */ {"RES 7 E", 2, {2, 2}},
    /* BC */ {"RES 7 H", 2, {2, 2}},
    /* BD */ {"RES 7 L", 2, {2, 2}},
    /* BE */ {"RES 7 (HL)", 2, {4, 4}},
    /* BF */ {"RES 7 A", 2, {2, 2}},
    /* C0 */ {"SET 0 B", 2, {2, 2}},
    /* C1 */ {"SET 0 C", 2, {2, 2}},
    /* C2 */ {"SET 0 D", 2, {2, 2}},
    /* C3 */ {"SET 0 E", 2, {2, 2}},
    /* C4 */ {"SET 0 H", 2, {2, 2}},
    /* C5 */ {"SET 0 L", 2, {2, 2}},
    /* C6 */ {"SET 0 (HL)", 2, {4, 4}},
    /* C7 */ {"SET 0 A", 2, {2, 2}},
    /* C8 */ {"SET 1 B", 2, {2, 2}},
    /* C9 */ {"SET 1 C", 2, {2, 2}},
    /* CA */ {"SET 1 D", 2, {2, 2}},
    /* CB */ {"SET 1 E", 2, {2, 2}},
    /* CC */ {"SET 1 H", 2, {2, 2}},
    /* CD */ {"SET 1 L", 2, {2, 2}},
    /* CE */ {"SET 1 (HL)", 2, {4, 4}},
    /* CF */ {"SET 1 A", 2, {2, 2}},
    /* D0 */ {"SET 2 B", 2, {2, 2}},
    /* D1 */ {"SET 2 C", 2, {2, 2}},
    /* D2 */ {"SET 2 D", 2, {2, 2}},
    /* D3 */ {"SET 2 E", 2, {2, 2}},
    /* D4 */ {"SET 2 H", 2, {2, 2}},
    /* D5 */ {"SET 2 L", 2, {2, 2}},
    /* D6 */ {"SET 2 (HL)", 2, {4, 4}},
    /* D7 */ {"SET 2 A", 2, {2, 2}},
    /* D8 */ {"SET 3 B", 2, {2, 2}},
    /* D9 */ {"SET 3 C", 2, {2, 2}},
    /* DA */ {"SET 3 D", 2, {2, 2}},
    /* DB */ {"SET 3 E", 2, {2, 2}},
    /* DC */ {"SET 3 H", 2, {2, 2}},
    /* DD */ {"SET 3 L", 2, {2, 2}},
    /* DE */ {"SET 3 (HL)", 2, {4, 4}},
    /* DF */ {"SET 3 A", 2, {2, 2}},
    /* E0 */ {"SET 4 B", 2, {2, 2}},
    /* E1 */ {"SET 4 C", 2, {2, 2}},
    /* E2 */ {"SET 4 D", 2, {2, 2}},
    /* E3 */ {"SET 4 E", 2, {2, 2}},
    /* E4 */ {"SET 4 H", 2, {2, 2}},
    /* E5 */ {"SET 4 L", 2, {2, 2}},
    /* E6 */ {"SET 4 (HL)", 2, {4, 4}},
    /* E7 */ {"SET 4 A", 2, {2, 2}},
    /* E8 */ {"SET 5 B", 2, {2, 2}},
    /* E9 */ {"SET 5 C", 2, {2, 2}},
    /* EA */ {"SET 5 D", 2, {2, 2}},
    /* EB */ {"SET 5 E", 2, {2, 2}},
    /* EC */ {"SET 5 H", 2, {2, 2}},
    /* ED */ {"SET 5 L", 2, {2, 2}},
    /* EE */ {"SET 5 (HL)", 2, {4, 4}},
    /* EF */ {"SET 5 A", 2, {2, 2}},
    /* F0 */ {"SET 6 B", 2, {2, 2}},
    /* F1 */ {"SET 6 C", 2, {2, 2}},
    /* F2 */ {"SET 6 D", 2, {2, 2}},
    /* F3 */ {"SET 6 E", 2, {2, 2}},
    /* F4 */ {"SET 6 H", 2, {2, 2}},
    /* F5 */ {"SET 6 L", 2, {2, 2}},
    /* F6 */ {"SET 6 (HL)", 2, {4, 4}},
    /* F7 */ {"SET 6 A", 2, {2, 2}},
    /* F8 */ {"SET 7 B", 2, {2, 2}},
    /* F9 */ {"SET 7 C", 2, {2, 2}},
    /* FA */ {"SET 7 D", 2, {2, 2}},
    /* FB */ {"SET 7 E", 2, {2, 2}},
    /* FC */ {"SET 7 H", 2, {2, 2}},
    /* FD */ {"SET 7 L", 2, {2, 2}},
    /* FE */ {"SET 7 (HL)", 2, {4, 4}},
    /* FF */ {"SET 7 A", 2, {2, 2}},
};

constexpr const char* IO_MNEMONICS[256] = {
    /* FF00 */ "P1",
    "SB",
    "SC",
    "UNKNOWN",
    "DIV",
    "TIMA",
    "TMA",
    "TAC",
    /* FF08 */ "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "IF",
    /* FF10 */ "NR10",
    "NR11",
    "NR12",
    "NR13",
    "NR14",
    "UNKNOWN",
    "NR21",
    "NR22",
    /* FF18 */ "NR23",
    "NR24",
    "NR30",
    "NR31",
    "NR32",
    "NR33",
    "NR34",
    "UNKNOWN",
    /* FF20 */ "NR41",
    "NR42",
    "NR43",
    "NR44",
    "NR50",
    "NR51",
    "NR52",
    "UNKNOWN",
    /* FF28 */ "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    /* FF30 */ "WAVE 0",
    "WAVE 1",
    "WAVE 2",
    "WAVE 3",
    "WAVE 4",
    "WAVE 5",
    "WAVE 6",
    "WAVE 7",
    /* FF38 */ "WAVE 8",
    "WAVE 9",
    "WAVE A",
    "WAVE B",
    "WAVE C",
    "WAVE D",
    "WAVE E",
    "WAVE F",
    /* FF40 */ "LCDC",
    "STAT",
    "SCY",
    "SCX",
    "LY",
    "LYC",
    "DMA",
    "BGP",
    /* FF48 */ "OBP0",
    "OBP1",
    "WY",
    "WX",
    "UNKNOWN",
    "KEY1",
    "UNKNOWN",
    "VBK",
    /* FF50 */ "BOOT",
    "HDMA SRC HI",
    "HDMA SRC LOW",
    "HDMA DST HI",
    "HDMA DST LOW",
    "HDMA LEN",
    "RP",
    "UNKNOWN",
    /* FF58 */ "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    /* FF60 */ "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    /* FF68 */ "BCPS",
    "BCPD",
    "OCPS",
    "OCPD",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    /* FF70 */ "SVBK",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "PCM12",
    "PCM34",
    /* FF78 */ "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
};

const std::map<std::string, uint16_t> IO_ADDRESSES = {
    {"P1", 0xFF00},    {"SB", 0xFF01},    {"SC", 0xFF02},    {"DIV", 0xFF04},   {"TIMA", 0xFF05},  {"TMA", 0xFF06},
    {"TAC", 0xFF07},   {"IF", 0xFF0F},    {"IE", 0xFFFF},    {"NR10", 0xFF10},  {"NR11", 0xFF11},  {"NR12", 0xFF12},
    {"NR13", 0xFF13},  {"NR14", 0xFF14},  {"NR21", 0xFF16},  {"NR22", 0xFF17},  {"NR23", 0xFF18},  {"NR24", 0xFF19},
    {"NR30", 0xFF1A},  {"NR31", 0xFF1B},  {"NR32", 0xFF1C},  {"NR33", 0xFF1D},  {"NR34", 0xFF1E},  {"NR41", 0xFF20},
    {"NR42", 0xFF21},  {"NR43", 0xFF22},  {"NR44", 0xFF23},  {"NR50", 0xFF24},  {"NR51", 0xFF25},  {"NR52", 0xFF26},
    {"WAVE0", 0xFF30}, {"WAVE1", 0xFF31}, {"WAVE2", 0xFF32}, {"WAVE3", 0xFF33}, {"WAVE4", 0xFF34}, {"WAVE5", 0xFF35},
    {"WAVE6", 0xFF36}, {"WAVE7", 0xFF37}, {"WAVE8", 0xFF38}, {"WAVE9", 0xFF39}, {"WAVEA", 0xFF3A}, {"WAVEB", 0xFF3B},
    {"WAVEC", 0xFF3C}, {"WAVED", 0xFF3D}, {"WAVEE", 0xFF3E}, {"WAVEF", 0xFF3F}, {"LCDC", 0xFF40},  {"STAT", 0xFF41},
    {"SCY", 0xFF42},   {"SCX", 0xFF43},   {"LY", 0xFF44},    {"LYC", 0xFF45},   {"DMA", 0xFF46},   {"BGP", 0xFF47},
    {"OBP0", 0xFF48},  {"OBP1", 0xFF49},  {"WY", 0xFF4A},    {"WX", 0xFF4B},    {"BOOT", 0xFF50},  {"PCM12", 0xFF76},
    {"PCM34", 0xFF77},
};

uint8_t instruction_length(uint8_t opcode) {
    return opcode == 0xCB ? 2 : INSTRUCTIONS[opcode].length;
}

std::pair<uint8_t, uint8_t> instruction_duration(const std::vector<uint8_t>& instruction) {
    if (instruction.empty()) {
        return {0, 0};
    }

    if (instruction[0] == 0xCB) {
        if (instruction.size() < 2) {
            return {0, 0};
        }
        return INSTRUCTIONS_CB[instruction[1]].duration;
    }

    return INSTRUCTIONS[instruction[0]].duration;
}

std::string instruction_mnemonic(const std::vector<uint8_t>& instruction, uint16_t address) {
    if (instruction.empty()) {
        return "[INVALID]";
    }
    uint8_t opcode = instruction[0];

    if (opcode == 0xCB) {
        if (instruction.size() < 2) {
            return "[INVALID]";
        }
        uint8_t opcode_CB = instruction[1];
        return INSTRUCTIONS_CB[opcode_CB].mnemonic;
    }

    switch (opcode) {
    // uu
    case 0x01:
    case 0x08:
    case 0x11:
    case 0x21:
    case 0x31:
    case 0xC2:
    case 0xC3:
    case 0xC4:
    case 0xCA:
    case 0xCC:
    case 0xCD:
    case 0xD2:
    case 0xD4:
    case 0xDA:
    case 0xDC:
    case 0xEA:
    case 0xFA: {
        if (instruction.size() < 3) {
            return "[INVALID]";
        }
        char buf[32];
        uint16_t uu = concat(instruction[2], instruction[1]);
        snprintf(buf, sizeof(buf), INSTRUCTIONS[opcode].mnemonic, uu);
        return buf;
    }
    // u
    case 0x06:
    case 0x0E:
    case 0x16:
    case 0x1E:
    case 0x26:
    case 0x2E:
    case 0x36:
    case 0x3E:
    case 0xC6:
    case 0xCE:
    case 0xD6:
    case 0xDE:
    case 0xE6:
    case 0xE8:
    case 0xEE:
    case 0xF6:
    case 0xF8:
    case 0xFE:

    {
        if (instruction.size() < 2) {
            return "[INVALID]";
        }
        char buf[32];
        snprintf(buf, sizeof(buf), INSTRUCTIONS[opcode].mnemonic, instruction[1]);
        return buf;
    }
    // (addr+s) s
    case 0x18:
    case 0x20:
    case 0x28:
    case 0x30:
    case 0x38: {
        if (instruction.size() < 2) {
            return "[INVALID]";
        }
        char buf[32];
        auto s = static_cast<int8_t>(instruction[1]);
        uint16_t uu = address + instruction.size() + s;
        snprintf(buf, sizeof(buf), INSTRUCTIONS[opcode].mnemonic, uu, s);
        return buf;
    }
    // u (FF00[u])
    case 0xE0:
    case 0xF0: {
        if (instruction.size() < 2) {
            return "[INVALID]";
        }
        char buf[32];
        snprintf(buf, sizeof(buf), INSTRUCTIONS[opcode].mnemonic, instruction[1],
                 address_to_mnemonic(0xFF00 + instruction[1]).c_str());
        return buf;
    }
    default:
        return INSTRUCTIONS[opcode].mnemonic;
    }
}

std::string address_to_mnemonic(uint16_t address) {
    if (address <= Specs::MemoryLayout::ROM0::END) {
        return "ROM0";
    }
    if (address <= Specs::MemoryLayout::ROM1::END) {
        return "ROM1";
    }
    if (address <= Specs::MemoryLayout::VRAM::END) {
        return "VRAM";
    }
    if (address <= Specs::MemoryLayout::RAM::END) {
        return "RAM";
    }
    if (address <= Specs::MemoryLayout::WRAM1::END) {
        return "WRAM1";
    }
    if (address <= Specs::MemoryLayout::WRAM2::END) {
        return "WRAM2";
    }
    if (address <= Specs::MemoryLayout::ECHO_RAM::END) {
        return "ECHO_RAM";
    }
    if (address <= Specs::MemoryLayout::OAM::END) {
        return "OAM";
    }
    if (address <= Specs::MemoryLayout::NOT_USABLE::END) {
        return "NOT_USABLE";
    }
    if (address <= Specs::MemoryLayout::IO::END) {
        return IO_MNEMONICS[address - Specs::MemoryLayout::IO::START];
    }
    if (address <= Specs::MemoryLayout::HRAM::END) {
        return "HRAM";
    }
    if (address == Specs::MemoryLayout::IE) {
        return "IE";
    }
    return hex(address);
}

std::optional<uint16_t> mnemonic_to_address(const std::string& mnemonic) {
    if (const auto it = IO_ADDRESSES.find(mnemonic); it != IO_ADDRESSES.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string interrupt_service_routine_mnemonic(uint16_t address) {
    switch (address) {
    case 0x40:
        return "VBLANK";
    case 0x48:
        return "LCD STAT";
    case 0x50:
        return "TIMER";
    case 0x58:
        return "SERIAL";
    case 0x60:
        return "JOYPAD";
    default:
        return "[INVALID]";
    }
}
