#ifndef BATTERIEDRAM_H
#define BATTERIEDRAM_H

#include "core/save/readable.h"
#include "core/save/writable.h"

class IBatteriedRAM {
public:
    virtual ~IBatteriedRAM() = default;

    virtual void loadRAM(IReadableSave& save) = 0;
    virtual void saveRAM(IWritableSave& save) = 0;
};

#endif // BATTERIEDRAM_H