#include "strings.hpp"
#include "testutils/img.h"
#include <iostream>

// usage: convframebuffer <in_file> <in_palette> <out_file> <out_palette>
// palette: HexColor1,HexColor2,HexColor2,HexColor3

int main(int argc, const char* argv[]) {
    if (argc < 5) {
        std::cerr << "usage: convframebuffer <in_file> <in_palette> <out_file> <out_palette>" << std::endl;
        exit(1);
    }

    // Read arguments
    std::string inFile {argv[1]};
    std::string inPaletteStr {argv[2]};
    std::string outFile {argv[3]};
    std::string outPaletteStr {argv[4]};

    // Parse palettes arguments into string tokens
    std::vector<std::string> inPaletteTokens;
    split(inPaletteStr, std::back_inserter(inPaletteTokens), [](char ch) {
        return ch == ',';
    });

    std::vector<std::string> outPaletteTokens;
    split(outPaletteStr, std::back_inserter(outPaletteTokens), [](char ch) {
        return ch == ',';
    });

    if (inPaletteTokens.size() != 4 || outPaletteTokens.size() != 4) {
        std::cerr << "ERROR: invalid palette." << std::endl;
        std::cerr << "Format is \"HexColor1,HexColor2,HexColor3,HexColor4\"" << std::endl;
        exit(1);
    }

    // Parse palettes string tokens into numbers
    const auto parsePalette = [](const std::vector<std::string>& tokens) {
        std::vector<uint16_t> palette;
        for (const auto& t : tokens) {
            palette.push_back(std::stol(t, nullptr, 16));
        }
        return palette;
    };

    std::vector<uint16_t> inPalette = parsePalette(inPaletteTokens);
    std::vector<uint16_t> outPalette = parsePalette(outPaletteTokens);

    // Read input file
    uint16_t inBuffer[FRAMEBUFFER_NUM_PIXELS];
    load_png_framebuffer(inFile, inBuffer);

    // Convert framebuffer
    uint16_t outBuffer[FRAMEBUFFER_NUM_PIXELS];
    convert_framebuffer_pixels(inBuffer, inPalette.data(), outBuffer, outPalette.data());

    // Save framebuffer to output file
    save_framebuffer_as_png(outFile, outBuffer);

    return 0;
}