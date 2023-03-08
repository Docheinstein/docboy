#ifndef BOOTROMFACTORY_H
#define BOOTROMFACTORY_H

#include <memory>
#include "core/impl/bootrom.h"

class BootROMFactory {
public:
    static std::unique_ptr<Impl::IBootROM> makeBootROM(const std::string &filename);
};

#endif // BOOTROMFACTORY_H