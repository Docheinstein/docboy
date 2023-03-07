#ifndef HELPERS_H
#define HELPERS_H

#include <optional>
#include "definitions.h"
#include <vector>

uint8_t instruction_length(uint8_t opcode, bool cb = false);
uint8_t instruction_length(const std::vector<uint8_t> &instruction);

std::pair<uint8_t, uint8_t> instruction_duration(uint8_t opcode, bool cb = false);
std::pair<uint8_t, uint8_t> instruction_duration(const std::vector<uint8_t> &instruction);

std::string instruction_mnemonic(uint8_t opcode, bool cb = false);
std::string instruction_mnemonic(const std::vector<uint8_t> &instruction, uint16_t address);

std::string interrupt_service_routine_mnemonic(uint16_t address);

std::string address_mnemonic(uint16_t address);

#endif // HELPERS_H