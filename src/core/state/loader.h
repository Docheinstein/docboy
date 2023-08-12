#ifndef STATELOADER_H
#define STATELOADER_H

#include "readable.h"

class IStateLoader {
public:
    ~IStateLoader() = default;
    virtual void loadState(IReadableState& state) = 0;
};

#endif // STATELOADER_H