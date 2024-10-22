#include "docboy/bootrom/factory.h"

#include "docboy/bootrom/bootrom.h"

#include "utils/io.h"

std::unique_ptr<BootRom> BootRomFactory::create(const std::string& filename) {
    bool ok;

    std::vector<uint8_t> data = read_file(filename, &ok);
    if (!ok) {
        FATAL("failed to read file");
    }

    ASSERT(data.size() == BootRom::Size);

    return std::make_unique<BootRom>(data.data());
}
