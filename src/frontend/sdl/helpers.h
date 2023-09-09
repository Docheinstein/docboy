#ifndef SDLHELPERS_H
#define SDLHELPERS_H

#include <cstdint>
#include <string>

[[maybe_unused]] bool screenshot(void* pixels, int width, int height, int format, const std::string& filename);

#endif // SDLHELPERS_H