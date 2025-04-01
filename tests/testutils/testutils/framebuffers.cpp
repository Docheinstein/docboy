#include "framebuffers.h"

#include "docboy/common/specs.h"
#include "extra/img/imgmanip.h"
#include "extra/png/iopng.h"
#include "utils/hexdump.h"
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

uint16_t convert_pixel_with_palette(uint16_t p, const std::array<uint16_t, 4>& inPalette,
                                    const std::array<uint16_t, 4>& outPalette) {
    for (uint8_t c = 0; c < 4; c++)
        if (p == inPalette[c])
            return outPalette[c];
    // checkNoEntry()
    return p;
}

void convert_framebuffer_with_palette(const uint16_t* in, const std::array<uint16_t, 4>& inPalette, uint16_t* out,
                                      const std::array<uint16_t, 4>& outPalette) {
    for (uint32_t i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
        out[i] = convert_pixel_with_palette(in[i], inPalette, outPalette);
    }
}

void compute_pixels_delta(uint16_t p1, uint16_t p2, int32_t& dr, int32_t& dg, int32_t& db) {
    uint8_t r1, g1, b1;
    pixel_to_rgb(p1, r1, g1, b1, ImageFormat::RGB565);

    uint8_t r2, g2, b2;
    pixel_to_rgb(p2, r2, g2, b2, ImageFormat::RGB565);

    dr = std::abs(r1 - r2);
    dg = std::abs(g1 - g2);
    db = std::abs(b1 - b2);
}

bool are_pixel_equals_with_tolerance(uint16_t p1, uint16_t p2, uint8_t rtol, uint8_t gtol, uint8_t btol) {
    int32_t dr, dg, db;
    compute_pixels_delta(p1, p2, dr, dg, db);
    return dr <= rtol && dg <= gtol && db <= btol;
}

bool are_framebuffer_equals(const uint16_t* buf1, const uint16_t* buf2) {
    return memcmp(buf1, buf2, FRAMEBUFFER_SIZE) == 0;
}

bool are_framebuffer_equals_with_tolerance(const uint16_t* buf1, const uint16_t* buf2, const uint8_t rtol,
                                           const uint8_t gtol, uint8_t const btol) {
    for (uint32_t i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
        if (!are_pixel_equals_with_tolerance(buf1[i], buf2[i], rtol, gtol, btol)) {
            return false;
        }
    }

    return true;
}

void dump_framebuffer_png(const std::string& filename) {
    uint16_t buffer[FRAMEBUFFER_NUM_PIXELS];
    load_framebuffer_png(filename, buffer);
    std::cout << hexdump(buffer, FRAMEBUFFER_NUM_PIXELS) << std::endl;
}