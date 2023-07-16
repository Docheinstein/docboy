#include "readable.h"

ReadOnlyMemory::ReadOnlyMemory(size_t size) :
    memory(size) {
}

ReadOnlyMemory::ReadOnlyMemory(const std::vector<uint8_t>& data) {
    memory = data;
}

ReadOnlyMemory::ReadOnlyMemory(std::vector<uint8_t>&& data) {
    memory = std::move(data);
}

uint8_t ReadOnlyMemory::read(uint16_t index) const {
    return memory[index];
}
