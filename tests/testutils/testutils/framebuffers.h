#ifndef FRAMEBUFFERS_H
#define FRAMEBUFFERS_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

using Palette = std::array<uint16_t, 4>;

constexpr uint32_t FRAMEBUFFER_NUM_PIXELS = 160 * 144;
constexpr uint32_t FRAMEBUFFER_SIZE = sizeof(uint16_t) * FRAMEBUFFER_NUM_PIXELS;

void load_framebuffer_png(const std::string& filename, uint16_t* buffer);
bool save_framebuffer_png(const std::string& filename, const uint16_t* buffer);

uint16_t convert_pixel_with_palette(uint16_t p, const Palette& inPalette, const Palette& outPalette);
void convert_framebuffer_with_palette(const uint16_t* in, const Palette& inPalette, uint16_t* out,
                                      const Palette& outPalette);

bool are_framebuffer_equals(const uint16_t* buf1, const uint16_t* buf2);

void dump_framebuffer_png(const std::string& filename);

#endif // FRAMEBUFFERS_H