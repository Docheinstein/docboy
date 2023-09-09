#ifndef WRITABLESAVE_H
#define WRITABLESAVE_H

#include <cstdint>
#include <vector>

class IWritableSave {
public:
    virtual ~IWritableSave() = default;

    virtual void writeData(uint8_t* data, uint32_t count) = 0;
};

#endif // WRITABLESAVE_H