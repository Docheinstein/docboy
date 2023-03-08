#include "readable.h"

Readable::Readable(size_t size) : memory(size) {

}

Readable::Readable(const std::vector<uint8_t> &data) {
    memory = data;
}

Readable::Readable(std::vector<uint8_t> &&data) {
    memory = std::move(data);
}

uint8_t Readable::read(uint16_t index) const {
    return memory[index];
}
