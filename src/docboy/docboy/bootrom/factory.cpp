#include "factory.h"
#include "utils/asserts.h"
#include "utils/exceptions.hpp"
#include "utils/io.h"

std::unique_ptr<BootRom> BootRomFactory::create(const std::string& filename) const {
    bool ok;

    std::vector<uint8_t> data = read_file(filename, &ok);
    if (!ok)
        fatal("failed to read file");

    check(data.size() < UINT16_MAX);

    return std::make_unique<BootRom>(data.data(), static_cast<uint16_t>(data.size()));
}
