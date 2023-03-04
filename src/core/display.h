#ifndef DISPLAY_H
#define DISPLAY_H

#include <cstdint>

class IDisplay {
public:
    typedef uint32_t Pixel;

    virtual ~IDisplay() = default;

    virtual void render(Pixel *pixels) = 0;
};
#endif // DISPLAY_H