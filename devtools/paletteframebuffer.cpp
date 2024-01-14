#include <algorithm>

#include "formatters.hpp"
#include "testutils/img.h"
#include <iostream>
#include <set>

// usage: paleteframebuffer <in_file>
int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: paleteframebuffer <in_file>" << std::endl;
        exit(1);
    }

    std::string file {argv[1]};

    uint16_t buffer[FRAMEBUFFER_NUM_PIXELS];
    load_png_framebuffer(file, buffer);

    std::set<uint16_t> colors;
    for (uint16_t& i : buffer) {
        colors.emplace(i);
    }

    std::vector<uint16_t> colorsVector;
    for (const auto& c : colors)
        colorsVector.push_back(c);

    std::sort(colorsVector.begin(), colorsVector.end(), std::greater());

    for (const auto color : colorsVector) {
        std::cout << hex(color) << std::endl;
    }

    return 0;
}