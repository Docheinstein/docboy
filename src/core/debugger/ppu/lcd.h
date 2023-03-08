#ifndef DEBUGGERLCD_H
#define DEBUGGERLCD_H

#include "core/ppu/lcd.h"


class IDebuggableLCD : public virtual ILCD {
public:

    ~IDebuggableLCD() override = default;

    [[nodiscard]] virtual uint8_t getX() const = 0;
    [[nodiscard]] virtual uint8_t getY() const = 0;
};

class DebuggableLCD : public IDebuggableLCD, public LCD {
    [[nodiscard]] uint8_t getX() const override;
    [[nodiscard]] uint8_t getY() const override;
};



#endif // DEBUGGERLCD_H