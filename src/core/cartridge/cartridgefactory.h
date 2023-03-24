#ifndef CARTRIDGEFACTORY_H
#define CARTRIDGEFACTORY_H

#include <memory>

class ICartridge;

class CartridgeFactory {
public:
    static std::unique_ptr<ICartridge> makeCartridge(const std::string &filename);
};

#endif // CARTRIDGEFACTORY_H