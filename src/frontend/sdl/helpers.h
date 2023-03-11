#ifndef SDLHELPERS_H
#define SDLHELPERS_H

#include <string>

[[maybe_unused]] bool screenshot(uint32_t *pixels, int width, int height, int format, const std::string &filename);

#endif // SDLHELPERS_H