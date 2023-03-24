#ifndef DEBUGGERLCD_H
#define DEBUGGERLCD_H

#include <cstdint>

class ILCDDebug {
public:
    virtual ~ILCDDebug() = default;

    struct State {
        uint8_t x;
        uint8_t y;
    };

    virtual State getState() = 0;
};

#endif // DEBUGGERLCD_H