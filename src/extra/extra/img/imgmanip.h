#ifndef IMGMANIP_H
#define IMGMANIP_H

#include <cstdint>
#include <vector>

enum class ImageFormat : uint8_t { RGB888, RGB565 };

void convert_image(ImageFormat from, const void* src, ImageFormat to, void* dst, uint32_t width, uint32_t height);

std::vector<uint8_t> create_image_buffer(uint32_t width, uint32_t height, ImageFormat fmt);

#endif // IMGMANIP_H