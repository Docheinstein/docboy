#ifndef BOOTROMFACTORY_H
#define BOOTROMFACTORY_H

#include "bootrom.h"
#include <memory>

class BootROMFactory {
public:
    static std::unique_ptr<IBootROM> makeBootROM(const std::string& filename);
};

#endif // BOOTROMFACTORY_H