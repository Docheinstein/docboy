#ifndef FRAMEBUFFERS_H
#define FRAMEBUFFERS_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

constexpr uint32_t FRAMEBUFFER_NUM_PIXELS = 160 * 144;
constexpr uint32_t FRAMEBUFFER_SIZE = sizeof(uint16_t) * FRAMEBUFFER_NUM_PIXELS;

bool load_framebuffer_png(const std::string& filename, uint16_t* buffer);
bool save_framebuffer_png(const std::string& filename, const uint16_t* buffer);

uint16_t convert_pixel_with_palette(uint16_t p, const std::array<uint16_t, 4>& inPalette,
                                    const std::array<uint16_t, 4>& outPalette);
void convert_framebuffer_with_palette(const uint16_t* in, const std::array<uint16_t, 4>& inPalette, uint16_t* out,
                                      const std::array<uint16_t, 4>& outPalette);

void compute_pixels_delta(uint16_t p1, uint16_t p2, int32_t& dr, int32_t& dg, int32_t& db);
bool are_pixel_equals_with_tolerance(uint16_t p1, uint16_t p2, uint8_t rtol, uint8_t gtol, uint8_t btol);

bool are_framebuffer_equals(const uint16_t* buf1, const uint16_t* buf2);
bool are_framebuffer_equals_with_tolerance(const uint16_t* buf1, const uint16_t* buf2, uint8_t rtol = 0,
                                           uint8_t gtol = 0, uint8_t btol = 0);

void dump_framebuffer_png(const std::string& filename);

#endif // FRAMEBUFFERS_H