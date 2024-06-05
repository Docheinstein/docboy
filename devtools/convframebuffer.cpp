#include <array>
#include <iostream>
#include <vector>

#include "testutils/framebuffers.h"
#include "utils/strings.h"

// usage: convframebuffer <in_file> <in_palette> <out_file> <out_palette>
// usage: convframebuffer <in_file> <in_palette> <out_palette> // in place
// palette: HexColor1,HexColor2,HexColor2,HexColor3

int main(int argc, const char* argv[]) {
    if (argc < 4) {
        std::cerr << "usage: convframebuffer <in_file> <in_palette> <out_file> <out_palette>" << std::endl;
        std::cerr << "usage: convframebuffer <in_file> <in_palette> <out_palette>" << std::endl;
        exit(1);
    }

    // Read arguments
    std::string in_file, in_palette_str, out_file, out_palette_str;

    if (argc == 4) {
        in_file = argv[1];
        in_palette_str = argv[2];
        out_file = in_file;
        out_palette_str = argv[3];
    } else {
        in_file = argv[1];
        in_palette_str = argv[2];
        out_file = argv[3];
        out_palette_str = argv[4];
    }

    // Parse palettes arguments into string tokens
    std::vector<std::string> in_palette_tokens;
    split(in_palette_str, std::back_inserter(in_palette_tokens), [](char ch) {
        return ch == ',';
    });

    std::vector<std::string> out_palette_tokens;
    split(out_palette_str, std::back_inserter(out_palette_tokens), [](char ch) {
        return ch == ',';
    });

    if (in_palette_tokens.size() != 4 || out_palette_tokens.size() != 4) {
        std::cerr << "ERROR: invalid palette." << std::endl;
        std::cerr << "Format is \"HexColor1,HexColor2,HexColor3,HexColor4\"" << std::endl;
        exit(1);
    }

    // Parse palettes string tokens into numbers
    const auto parse_palette = [](const std::vector<std::string>& tokens) {
        std::array<uint16_t, 4> palette {};
        for (uint32_t i = 0; i < 4 && i < tokens.size(); i++) {
            palette[i] = std::stol(tokens[i], nullptr, 16);
        }
        return palette;
    };

    std::array<uint16_t, 4> in_palette = parse_palette(in_palette_tokens);
    std::array<uint16_t, 4> out_palette = parse_palette(out_palette_tokens);

    // Read input file
    uint16_t in_buffer[FRAMEBUFFER_NUM_PIXELS];
    load_framebuffer_png(in_file, in_buffer);

    // Convert framebuffer
    uint16_t out_buffer[FRAMEBUFFER_NUM_PIXELS];
    convert_framebuffer_with_palette(in_buffer, in_palette, out_buffer, out_palette);

    // Save framebuffer to output file
    save_framebuffer_png(out_file, out_buffer);

    return 0;
}