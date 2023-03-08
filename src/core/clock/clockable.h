#ifndef CLOCKABLE_H
#define CLOCKABLE_H

class IClockable {
public:
    virtual ~IClockable() = default;
    virtual void tick() = 0;
};

#endif // CLOCKABLE_H