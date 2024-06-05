#ifndef BOOTROMFACTORY_H
#define BOOTROMFACTORY_H

#include <memory>
#include <string>

#include "docboy/bootrom/fwd/bootromfwd.h"

namespace BootRomFactory {
std::unique_ptr<BootRom> create(const std::string& filename);
};

#endif // BOOTROMFACTORY_H