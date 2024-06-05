#include "utils/memory.h"

std::optional<uint32_t> mem_find_first(const uint8_t* haystack, uint32_t haystack_size, const uint8_t* needle,
                                       uint32_t needle_size) {
    for (uint32_t h = 0; h < haystack_size; h++) {
        uint32_t i = 0;
        while (i < needle_size && h + i < haystack_size && needle[i] == haystack[h + i]) {
            ++i;
        }
        if (i == needle_size) {
            return h;
        }
    }
    return std::nullopt;
}

std::vector<uint32_t> mem_find_all(const uint8_t* haystack, uint32_t haystack_size, const uint8_t* needle,
                                   uint32_t needle_size) {
    std::vector<uint32_t> occurrences;

    for (uint32_t h = 0; h < haystack_size; h++) {
        uint32_t i = 0;
        while (i < needle_size && h + i < haystack_size && needle[i] == haystack[h + i]) {
            ++i;
        }
        if (i == needle_size) {
            occurrences.push_back(h);
        }
    }

    return occurrences;
}
