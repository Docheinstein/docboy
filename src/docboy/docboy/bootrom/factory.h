#ifndef BOOTROMFACTORY_H
#define BOOTROMFACTORY_H

#include <memory>
#include <string>

#include "docboy/bootrom/fwd/bootromfwd.h"

namespace BootRomFactory {
void load(BootRom& boot_rom, const std::string& filename);
};

#endif // BOOTROMFACTORY_H