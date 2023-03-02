#ifndef CARTRIDGEFACTORY_H
#define CARTRIDGEFACTORY_H

#include "cartridge.h"
#include <memory>

class CartridgeFactory {
public:
    static std::unique_ptr<Cartridge> makeCartridge(const std::string &filename);
};

#endif // CARTRIDGEFACTORY_H