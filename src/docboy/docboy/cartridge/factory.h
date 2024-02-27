#ifndef CARTRIDGEFACTORY_H
#define CARTRIDGEFACTORY_H

#include "cartridge.h"
#include <memory>
#include <string>
#include <vector>

class CartridgeFactory {
public:
    [[nodiscard]] std::unique_ptr<ICartridge> create(const std::string& filename) const;
};

#endif // CARTRIDGEFACTORY_H