#ifndef BOOTROMFACTORY_H
#define BOOTROMFACTORY_H

#include "bootrom.h"
#include <memory>
#include <string>

class BootRomFactory {
public:
    [[nodiscard]] std::unique_ptr<BootRom> create(const std::string& filename) const;
};

#endif // BOOTROMFACTORY_H