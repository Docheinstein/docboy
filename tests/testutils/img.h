#ifndef IMGUTILS_H
#define IMGUTILS_H

#include <cstdint>
#include <string>
#include <vector>

static constexpr uint32_t FRAMEBUFFER_NUM_PIXELS = 160 * 144;
static constexpr uint32_t FRAMEBUFFER_SIZE = sizeof(uint16_t) * FRAMEBUFFER_NUM_PIXELS;

void load_png(const std::string& filename, void* buffer, int format = -1, uint32_t* size = nullptr);
void save_png(const std::string& filename, void* buffer, int width, int height);

void load_png_framebuffer(const std::string& filename, uint16_t* buffer);
bool save_framebuffer_as_png(const std::string& filename, const uint16_t* buffer);

uint16_t convert_framebuffer_pixel(uint16_t p, const uint16_t* inPalette, const uint16_t* outPalette);
void convert_framebuffer_pixels(const uint16_t* in, const uint16_t* inPalette, uint16_t* out,
                                const uint16_t* outPalette);

bool are_framebuffer_equals(const uint16_t* buf1, const uint16_t* buf2);

void dump_png_framebuffer(const std::string& filename);

#endif // IMGUTILS_H