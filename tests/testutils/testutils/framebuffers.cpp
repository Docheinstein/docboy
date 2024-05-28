#include "framebuffers.h"

#include "docboy/shared/specs.h"
#include "extra/img/imgmanip.h"
#include "extra/png/iopng.h"
#include "utils/hexdump.hpp"
#include <cstring>

void load_framebuffer_png(const std::string& filename, uint16_t* buffer) {
    const auto buffer_rgb888 = load_png_rgb888(filename);
    convert_image(ImageFormat::RGB888, buffer_rgb888.data(), ImageFormat::RGB565, buffer, Specs::Display::WIDTH,
                  Specs::Display::HEIGHT);
}

bool save_framebuffer_png(const std::string& filename, const uint16_t* buffer) {
    auto buffer_rgb888 = create_image_buffer(Specs::Display::WIDTH, Specs::Display::HEIGHT, ImageFormat::RGB888);
    convert_image(ImageFormat::RGB565, buffer, ImageFormat::RGB888, buffer_rgb888.data(), Specs::Display::WIDTH,
                  Specs::Display::HEIGHT);
    return save_png_rgb888(filename, buffer_rgb888.data(), Specs::Display::WIDTH, Specs::Display::HEIGHT);
}

uint16_t convert_pixel_with_palette(uint16_t p, const Palette& inPalette, const Palette& outPalette) {
    for (uint8_t c = 0; c < 4; c++)
        if (p == inPalette[c])
            return outPalette[c];
    // checkNoEntry()
    return p;
}

void convert_framebuffer_with_palette(const uint16_t* in, const Palette& inPalette, uint16_t* out,
                                      const Palette& outPalette) {
    for (uint32_t i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
        out[i] = convert_pixel_with_palette(in[i], inPalette, outPalette);
    }
}

bool are_framebuffer_equals(const uint16_t* buf1, const uint16_t* buf2) {
    return memcmp(buf1, buf2, FRAMEBUFFER_SIZE) == 0;
}

void dump_framebuffer_png(const std::string& filename) {
    uint16_t buffer[FRAMEBUFFER_NUM_PIXELS];
    load_framebuffer_png(filename, buffer);
    std::cout << hexdump(buffer, FRAMEBUFFER_NUM_PIXELS) << std::endl;
}