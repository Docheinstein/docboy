#include "memory.h"

Memory::Memory(size_t size) : memory(size) {

}

Memory::Memory(const std::vector<uint8_t> &data) {
    memory = data;
}

Memory::Memory(std::vector<uint8_t> &&data) {
    memory = std::move(data);
}

uint8_t Memory::read(uint16_t index) const {
    return memory[index];
}

void Memory::write(uint16_t index, uint8_t value) {
    memory[index] = value;
}
