#ifndef CARTRIDGEFACTORY_H
#define CARTRIDGEFACTORY_H

#include <memory>
#include <string>

#include "docboy/cartridge/header.h"

class ICartridge;

namespace CartridgeFactory {
std::pair<std::unique_ptr<ICartridge>, CartridgeHeader> create(const std::string& filename);
CartridgeHeader create_header(const std::string& filename);
}; // namespace CartridgeFactory

#endif // CARTRIDGEFACTORY_H