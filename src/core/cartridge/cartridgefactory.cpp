#include "cartridgefactory.h"
#include "cartridge.h"
#include "headeronlycartridge.h"
#include "mbc1/mbc1.h"
#include "mbc1/mbc1ram.h"
#include "mbc3/mbc3.h"
#include "mbc3/mbc3ram.h"
#include "nombc.h"
#include "utils/binutils.h"
#include "utils/fileutils.h"

CartridgeFactory::CartridgeFactory() :
    makeHeaderOnlyCartridgeForUnknownType() {
}

void CartridgeFactory::setMakeHeaderOnlyCartridgeForUnknownType(bool yes) {
    makeHeaderOnlyCartridgeForUnknownType = yes;
}

std::unique_ptr<ICartridge> CartridgeFactory::makeCartridge(const std::string& filename) const {
    bool ok;

    std::vector<uint8_t> data = read_file(filename, &ok);
    if (!ok)
        throw std::runtime_error("failed to read file");

    if (data.size() < 0x0150)
        throw std::runtime_error("rom size is too small");

    uint8_t mbc = data[0x147];

    if (mbc == 0x00)
        return std::make_unique<NoMBC>(data);
    if (mbc == 0x01)
        return std::make_unique<MBC1>(data);
    if (mbc == 0x02 || mbc == 0x03)
        return std::make_unique<MBC1RAM>(data);
    if (mbc == 0x11 || mbc == 0x0F)
        return std::make_unique<MBC3>(data);
    if (mbc == 0x12 || mbc == 0x13 || mbc == 0x10)
        return std::make_unique<MBC3RAM>(data);

    if (makeHeaderOnlyCartridgeForUnknownType)
        return std::make_unique<HeaderOnlyCartridge>(data);

    throw std::runtime_error("unknown MBC type: 0x" + hex(mbc));
}
