#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <optional>
#include <vector>

/* Find first occurrence of needle in haystack or NOT_FOUND if it's not found. */
std::optional<uint32_t> mem_find_first(const uint8_t* haystack, uint32_t haystack_size, const uint8_t* needle,
                                       uint32_t needle_size);

/* Find first occurrence of needle in haystack or NOT_FOUND if it's not found. */
std::vector<uint32_t> mem_find_all(const uint8_t* haystack, uint32_t haystack_size, const uint8_t* needle,
                                   uint32_t needle_size);

#endif // MEMORY_H