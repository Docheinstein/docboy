#ifndef LCDAPPEARANCE_H
#define LCDAPPEARANCE_H

#include <array>
#include <cstdint>

using PixelRgb565 = uint16_t;

struct Appearance {
#ifdef ENABLE_CGB
    static constexpr uint16_t NUM_COLORS = 32768;
#else
    static constexpr uint16_t NUM_COLORS = 4;
#endif

    using Palette = std::array<PixelRgb565, NUM_COLORS>;

    PixelRgb565 default_color {};
    Palette palette {};
};

#endif // LCDAPPEARANCE_H
