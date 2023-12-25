#include "formatters.hpp"
#include "strings.hpp"
#include "testutils/img.h"
#include <iostream>

// usage: convframebuffer <in_file> <in_palette> <out_file> <out_palette>

int main(int argc, const char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: cmpframebuffers <in_file_1> <in_file_2>" << std::endl;
        exit(1);
    }

    // Read arguments
    std::string inFile1 {argv[1]};
    std::string inFile2 {argv[2]};

    // Read files
    uint16_t inBuffer1[FRAMEBUFFER_NUM_PIXELS];
    load_png_framebuffer(inFile1, inBuffer1);

    uint16_t inBuffer2[FRAMEBUFFER_NUM_PIXELS];
    load_png_framebuffer(inFile2, inBuffer2);

    uint32_t i;
    for (i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
        if (inBuffer1[i] != inBuffer2[i]) {
            std::cerr << "Mismatch at position " << i << " (row=" << i / 160 << ", column=" << i % 160 << "): 0x"
                      << hex(inBuffer1[i]) << " != 0x" << hex(inBuffer2[i]) << std::endl;
            return 1;
        }
    }

    return 0;
}