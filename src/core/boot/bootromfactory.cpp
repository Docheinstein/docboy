#include "bootromfactory.h"
#include "utils/ioutils.h"

std::unique_ptr<IBootROM> BootROMFactory::makeBootROM(const std::string& filename) {
    bool ok;

    std::vector<uint8_t> data = read_file(filename, &ok);
    if (!ok)
        return nullptr;

    return std::make_unique<BootROM>(data);
}