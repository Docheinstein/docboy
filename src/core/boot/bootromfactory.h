#ifndef BOOTROMFACTORY_H
#define BOOTROMFACTORY_H

#include <memory>
#include "core/components.h"


class BootROMFactory {
public:
    static std::unique_ptr<IReadableImpl> makeBootROM(const std::string &filename);
};

#endif // BOOTROMFACTORY_H