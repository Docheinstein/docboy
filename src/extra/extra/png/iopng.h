#ifndef IOPNG_H
#define IOPNG_H

#include <cstdint>
#include <string>
#include <vector>

bool save_png_rgb888(const std::string& filename, const void* buffer, uint32_t width, uint32_t height);
std::vector<uint8_t> load_png_rgb888(const std::string& filename, uint32_t* width = nullptr, uint32_t* height = nullptr,
                                     bool* ok = nullptr);

#endif // IOPNG_H