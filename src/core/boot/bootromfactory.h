#ifndef BOOTROMFACTORY_H
#define BOOTROMFACTORY_H

#include <memory>
#include "bootrom.h"

class BootROMFactory {
public:
    static std::unique_ptr<BootROM> makeBootROM(const std::string &filename);
};

#endif // BOOTROMFACTORY_H