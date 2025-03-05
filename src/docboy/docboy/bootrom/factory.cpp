#include "docboy/bootrom/factory.h"

#include <cstring>

#include "docboy/bootrom/bootrom.h"

#include "utils/io.h"

void BootRomFactory::load(BootRom& boot_rom, const std::string& filename) {
    bool ok;

    std::vector<uint8_t> data = read_file(filename, &ok);
    if (!ok) {
        FATAL("failed to read file");
    }

    ASSERT(data.size() == BootRom::Size);

    memcpy(boot_rom.data, data.data(), data.size());
}
