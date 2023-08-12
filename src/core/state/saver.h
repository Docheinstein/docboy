#ifndef STATESAVER_H
#define STATESAVER_H

#include "writable.h"

class IStateSaver {
public:
    ~IStateSaver() = default;
    virtual void saveState(IWritableState& state) = 0;
};

#endif // STATESAVER_H