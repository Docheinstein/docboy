#include <iostream>

#include "testutils/framebuffers.h"
#include "utils/formatters.h"
#include "utils/strings.h"

// usage: convframebuffer <in_file> <in_palette> <out_file> <out_palette>

int main(int argc, const char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: cmpframebuffers <in_file_1> <in_file_2>" << std::endl;
        exit(1);
    }

    // Read arguments
    std::string in_file {argv[1]};
    std::string in_file2 {argv[2]};

    // Read files
    uint16_t in_buffer1[FRAMEBUFFER_NUM_PIXELS];
    load_framebuffer_png(in_file, in_buffer1);

    uint16_t in_buffer2[FRAMEBUFFER_NUM_PIXELS];
    load_framebuffer_png(in_file2, in_buffer2);

    uint32_t i;
    for (i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
        if (in_buffer1[i] != in_buffer2[i]) {
            std::cerr << "Mismatch at position " << i << " (row=" << i / 160 << ", column=" << i % 160 << "): 0x"
                      << hex(in_buffer1[i]) << " != 0x" << hex(in_buffer2[i]) << std::endl;
            return 1;
        }
    }

    return 0;
}