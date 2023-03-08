#include "bootromfactory.h"
#include "utils/fileutils.h"

std::unique_ptr<Impl::IBootROM> BootROMFactory::makeBootROM(const std::string &filename) {
    bool ok;

    std::vector<uint8_t> data = read_file(filename, &ok);
    if (!ok)
        return nullptr;

    return std::make_unique<Impl::BootROM>(data);
}
