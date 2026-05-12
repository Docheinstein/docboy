#ifndef TEST_MNEMONICS_H
#define TEST_MNEMONICS_H

#include <cstdint>
#include <optional>
#include <string>

std::optional<uint8_t> instruction_mnemonic_to_opcode(const std::string& mnemonic);

#endif // TEST_MNEMONICS_H
