#ifndef CARTRIDGEFACTORY_H
#define CARTRIDGEFACTORY_H

#include <memory>

class ICartridge;

class CartridgeFactory {
public:
    CartridgeFactory();

    void setMakeHeaderOnlyCartridgeForUnknownType(bool yes);

    [[nodiscard]] std::unique_ptr<ICartridge> makeCartridge(const std::string& filename) const;

private:
    bool makeHeaderOnlyCartridgeForUnknownType;
};

#endif // CARTRIDGEFACTORY_H