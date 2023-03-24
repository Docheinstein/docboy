#include "cartridgefactory.h"
#include "cartridge.h"
#include "utils/fileutils.h"
#include "nombc.h"
#include "mbc1.h"
#include "mbc1ram.h"

std::unique_ptr<ICartridge>
CartridgeFactory::makeCartridge(const std::string &filename) {
    bool ok;

    std::vector<uint8_t> data = read_file(filename, &ok);
    if (!ok)
        return nullptr;

    if (data.size() < 0x0150)
        return nullptr;

    uint8_t mbc = data[0x147];

    if (mbc == 0x00)
        return std::make_unique<NoMBC>(data);
    if (mbc == 0x01)
        return std::make_unique<MBC1>(data);
    if (mbc == 0x02 || mbc == 0x03)
        return std::make_unique<MBC1RAM>(data);

    return nullptr;
}