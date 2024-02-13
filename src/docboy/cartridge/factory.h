#ifndef CARTRIDGEFACTORY_H
#define CARTRIDGEFACTORY_H

#include "cartridge.h"
#include <memory>
#include <string>
#include <vector>

class CartridgeFactory {
public:
    [[nodiscard]] std::unique_ptr<ICartridge> create(const std::string& filename) const;

private:
    [[nodiscard]] std::unique_ptr<ICartridge> createNoMbc(const std::vector<uint8_t>& data, uint8_t rom,
                                                          uint8_t ram) const;

    template <bool Battery>
    [[nodiscard]] std::unique_ptr<ICartridge> createMbc1(const std::vector<uint8_t>& data, uint8_t rom,
                                                         uint8_t ram) const;

    template <bool Battery, bool Timer>
    [[nodiscard]] std::unique_ptr<ICartridge> createMbc3(const std::vector<uint8_t>& data, uint8_t rom,
                                                         uint8_t ram) const;

    template <bool Battery>
    [[nodiscard]] std::unique_ptr<ICartridge> createMbc5(const std::vector<uint8_t>& data, uint8_t rom,
                                                         uint8_t ram) const;
};

#endif // CARTRIDGEFACTORY_H