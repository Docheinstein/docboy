#include "cartridgefactory.h"
#include "cartridge.h"
#include "core/cartridge/mbc1/mbc1rambattery.h"
#include "core/cartridge/mbc3/mbc3rambattery.h"
#include "headeronlycartridge.h"
#include "mbc1/mbc1.h"
#include "mbc1/mbc1ram.h"
#include "mbc3/mbc3.h"
#include "mbc3/mbc3ram.h"
#include "nombc/nombc.h"
#include "utils/binutils.h"
#include "utils/exceptionutils.h"
#include "utils/ioutils.h"

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
        THROW(std::runtime_error("failed to read file"));

    if (data.size() < 0x0150)
        THROW(std::runtime_error("rom size is too small"));

    uint8_t mbc = data[0x147];

    if (mbc == 0x00)
        return std::make_unique<NoMBC>(data);
    if (mbc == 0x01)
        return std::make_unique<MBC1>(data);
    if (mbc == 0x02)
        return std::make_unique<MBC1RAM>(data);
    if (mbc == 0x03)
        return std::make_unique<MBC1RAMBattery>(data);
    if (mbc == 0x11 || mbc == 0x0F)
        return std::make_unique<MBC3>(data);
    if (mbc == 0x12)
        return std::make_unique<MBC3RAM>(data);
    if (mbc == 0x10 || mbc == 0x13)
        return std::make_unique<MBC3RAMBattery>(data);

    if (makeHeaderOnlyCartridgeForUnknownType)
        return std::make_unique<HeaderOnlyCartridge>(data);

    THROW(std::runtime_error("unknown MBC type: 0x" + hex(mbc)));
}
