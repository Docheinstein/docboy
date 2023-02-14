#include "memory.h"
#include "log/log.h"

IMemory::~IMemory() {

}

Memory::Memory() : memory() {

}

uint8_t &Memory::operator[](size_t index) {
    DEBUG(1) << "Memory[" << index << "]" << std::endl;
    return memory[index];
}

const uint8_t &Memory::operator[](size_t index) const {
    DEBUG(1) << "Memory[" << index << "]" << std::endl;
    return memory[index];
}
