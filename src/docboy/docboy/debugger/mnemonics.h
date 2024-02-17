#ifndef MNEMONICS_H
#define MNEMONICS_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

uint8_t instruction_length(uint8_t opcode);
std::pair<uint8_t, uint8_t> instruction_duration(const std::vector<uint8_t>& instruction);
std::string instruction_mnemonic(const std::vector<uint8_t>& instruction, uint16_t address);

std::string address_to_mnemonic(uint16_t address);
std::optional<uint16_t> mnemonic_to_address(const std::string& mnemonic);
std::string interrupt_service_routine_address_to_mnemonic(uint16_t address);

#endif // MNEMONICS_H