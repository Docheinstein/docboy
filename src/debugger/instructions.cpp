#include "instructions.h"
#include "utils/binutils.h"

std::string instruction_to_string(std::vector<uint8_t> instruction) {
    if (instruction.empty())
        return "[INVALID]";
    uint8_t opcode = instruction[0];
    
    if (opcode == 0xCB) {
        if (instruction.size() < 2)
            return "[INVALID]";
        uint8_t opcodeCB = instruction[1];
        return INSTRUCTIONS_CB[opcodeCB].mnemonic;
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
        case 0xFA:
        {
            if (instruction.size() < 3)
                return "[INVALID]";
            char buf[32];
            uint16_t uu = concat_bytes(instruction[2], instruction[1]);
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
        case 0xE0:
        case 0xE6:
        case 0xEE:
        case 0xF0:
        case 0xF6:
        case 0xFE:
        {
            if (instruction.size() < 2)
                return "[INVALID]";
            char buf[32];
            snprintf(buf, sizeof(buf), INSTRUCTIONS[opcode].mnemonic, instruction[1]);
            return buf;
        }
        // s
        case 0x18:
        case 0x20:
        case 0x28:
        case 0x30:
        case 0x38:
        case 0xE8:
        case 0xF8:
        {
            if (instruction.size() < 2)
                return "[INVALID]";
            char buf[32];
            snprintf(buf, sizeof(buf), INSTRUCTIONS[opcode].mnemonic, static_cast<int8_t>(instruction[1]));
            return buf;
        }
        default:
            return INSTRUCTIONS[opcode].mnemonic;
    }
}
