#include <algorithm>
#include <iostream>
#include <set>

#include "testutils/framebuffers.h"
#include "utils/formatters.h"

// usage: paleteframebuffer <in_file>

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: paleteframebuffer <in_file>" << std::endl;
        exit(1);
    }

    std::string file {argv[1]};

    uint16_t buffer[FRAMEBUFFER_NUM_PIXELS];
    load_framebuffer_png(file, buffer);

    std::set<uint16_t> colors;
    for (uint16_t& i : buffer) {
        colors.emplace(i);
    }

    std::vector<uint16_t> colors_vector;
    for (const auto& c : colors)
        colors_vector.push_back(c);

    std::sort(colors_vector.begin(), colors_vector.end(), std::greater());

    for (const auto color : colors_vector) {
        std::cout << hex(color) << std::endl;
    }

    return 0;
}