#ifndef IMGMANIP_H
#define IMGMANIP_H

#include <cstdint>
#include <vector>

enum class ImageFormat : uint8_t { RGB888, RGB565 };

uint32_t rgb_to_pixel(uint8_t r, uint8_t g, uint8_t b, ImageFormat fmt);

void pixel_to_rgb(uint32_t pixel, uint8_t& r, uint8_t& g, uint8_t& b, ImageFormat fmt);

uint32_t convert_pixel(uint32_t pixel, ImageFormat from, ImageFormat to);

void convert_image(ImageFormat from, const void* src, ImageFormat to, void* dst, uint32_t width, uint32_t height);

std::vector<uint8_t> create_image_buffer(uint32_t width, uint32_t height, ImageFormat fmt);

#endif // IMGMANIP_H