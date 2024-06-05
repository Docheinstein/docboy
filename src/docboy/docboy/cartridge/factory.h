#ifndef CARTRIDGEFACTORY_H
#define CARTRIDGEFACTORY_H

#include <memory>
#include <string>
#include <vector>

class ICartridge;

namespace CartridgeFactory {
std::unique_ptr<ICartridge> create(const std::string& filename);
};

#endif // CARTRIDGEFACTORY_H