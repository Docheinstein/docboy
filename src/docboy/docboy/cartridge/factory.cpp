#include "factory.h"
#include "docboy/cartridge/mbc1/mbc1.h"
#include "docboy/cartridge/mbc1/mbc1m.h"
#include "docboy/cartridge/mbc2/mbc2.h"
#include "docboy/cartridge/mbc3/mbc3.h"
#include "docboy/cartridge/mbc5/mbc5.h"
#include "docboy/cartridge/nombc/nombc.h"
#include "docboy/shared/specs.h"
#include "utils/exceptions.hpp"
#include "utils/formatters.hpp"
#include "utils/io.h"

namespace {
constexpr bool Battery = true;
constexpr bool NoBattery = false;

constexpr bool Timer = true;
constexpr bool NoTimer = false;

constexpr uint32_t KB = 1 << 10;
constexpr uint32_t MB = 1 << 20;

using namespace Specs::Cartridge::Header;

bool isMbc1M(const std::vector<uint8_t>& data) {
    // MBC1M can't be distinguished from MBC1 only from the header.
    // The heuristic for distinguish it is the presence of (at least two)
    // NINTENDO LOGO at the beginning of each game's bank.
    if (data.size() < 0x100000 ||
        (data[MemoryLayout::TYPE] != Mbc::MBC1 && data[MemoryLayout::TYPE] != Mbc::MBC1_RAM &&
         data[MemoryLayout::TYPE] != Mbc::MBC1_RAM_BATTERY) ||
        data[MemoryLayout::ROM_SIZE] != Rom::MB_1)
        return false;

    // Count the occurrences of Nintendo Logo.
    uint8_t numLogo {};
    for (uint8_t i = 0; i < 4; i++) {
        numLogo +=
            memcmp(data.data() + (0x40000 * i) + MemoryLayout::LOGO::START, NINTENDO_LOGO, sizeof(NINTENDO_LOGO)) == 0;
    }

    return numLogo > 1;
}

std::unique_ptr<ICartridge> createNoMbc(const std::vector<uint8_t>& data, uint8_t rom, uint8_t ram) {
    if (rom == Rom::KB_32) {
        if (ram == Ram::KB_8)
            return std::make_unique<NoMbc<8 * KB>>(data.data(), data.size());
        if (ram == Ram::NONE)
            return std::make_unique<NoMbc<0>>(data.data(), data.size());
    }

    fatal("NoMbc: unexpected data in cartridge header: rom=" + std::to_string(rom) + ", ram=" + std::to_string(ram));
}

template <bool Battery>
std::unique_ptr<ICartridge> createMbc1(const std::vector<uint8_t>& data, uint8_t rom, uint8_t ram) {
    if (rom == Rom::KB_32) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc1<32 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc1<32 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc1<32 * KB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::KB_64) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc1<64 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc1<64 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc1<64 * KB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::KB_128) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc1<128 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc1<128 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc1<128 * KB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::KB_256) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc1<256 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc1<256 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc1<256 * KB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::KB_512) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc1<512 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc1<512 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc1<512 * KB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::MB_1) {
        // All the known MBC1M are 1 MB.
        if (isMbc1M(data)) {
            if (ram == Ram::NONE)
                return std::make_unique<Mbc1M<1 * MB, 0, Battery>>(data.data(), data.size());
            if (ram == Ram::KB_8)
                return std::make_unique<Mbc1M<1 * MB, 8 * KB, Battery>>(data.data(), data.size());
            if (ram == Ram::KB_32)
                return std::make_unique<Mbc1M<1 * MB, 32 * KB, Battery>>(data.data(), data.size());
        } else {
            if (ram == Ram::NONE)
                return std::make_unique<Mbc1<1 * MB, 0, Battery>>(data.data(), data.size());
            if (ram == Ram::KB_8)
                return std::make_unique<Mbc1<1 * MB, 8 * KB, Battery>>(data.data(), data.size());
            if (ram == Ram::KB_32)
                return std::make_unique<Mbc1<1 * MB, 32 * KB, Battery>>(data.data(), data.size());
        }
    } else if (rom == Rom::MB_2) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc1<2 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc1<2 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc1<2 * MB, 32 * KB, Battery>>(data.data(), data.size());
    }

    fatal("Mbc1: unexpected data in cartridge header: rom=" + std::to_string(rom) + ", ram=" + std::to_string(ram));
}

template <bool Battery>
std::unique_ptr<ICartridge> createMbc2(const std::vector<uint8_t>& data, uint8_t rom, uint8_t ram) {
    if (ram == Ram::NONE) {
        if (rom == Rom::KB_32)
            return std::make_unique<Mbc2<32 * KB, Battery>>(data.data(), data.size());
        if (rom == Rom::KB_64)
            return std::make_unique<Mbc2<64 * KB, Battery>>(data.data(), data.size());
        if (rom == Rom::KB_128)
            return std::make_unique<Mbc2<128 * KB, Battery>>(data.data(), data.size());
        if (rom == Rom::KB_256)
            return std::make_unique<Mbc2<256 * KB, Battery>>(data.data(), data.size());
    }

    fatal("Mbc2: unexpected data in cartridge header: rom=" + std::to_string(rom) + ", ram=" + std::to_string(ram));
}

template <bool Battery, bool Timer>
std::unique_ptr<ICartridge> createMbc3(const std::vector<uint8_t>& data, uint8_t rom, uint8_t ram) {
    if (rom == Rom::KB_32) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc3<32 * KB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc3<32 * KB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc3<32 * KB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Rom::KB_64) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc3<64 * KB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc3<64 * KB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc3<64 * KB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Rom::KB_128) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc3<128 * KB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc3<128 * KB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc3<128 * KB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Rom::KB_256) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc3<256 * KB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc3<256 * KB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc3<256 * KB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Rom::KB_512) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc3<512 * KB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc3<512 * KB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc3<512 * KB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Rom::MB_1) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc3<1 * MB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc3<1 * MB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc3<1 * MB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Rom::MB_2) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc3<2 * MB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc3<2 * MB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc3<2 * MB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    }

    fatal("Mbc3: unexpected data in cartridge header: rom=" + std::to_string(rom) + ", ram=" + std::to_string(ram));
}

template <bool Battery>
std::unique_ptr<ICartridge> createMbc5(const std::vector<uint8_t>& data, uint8_t rom, uint8_t ram) {
    if (rom == Rom::KB_32) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc5<32 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc5<32 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc5<32 * KB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_64)
            return std::make_unique<Mbc5<32 * KB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_128)
            return std::make_unique<Mbc5<32 * KB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::KB_64) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc5<64 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc5<64 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc5<64 * KB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_64)
            return std::make_unique<Mbc5<64 * KB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_128)
            return std::make_unique<Mbc5<64 * KB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::KB_128) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc5<128 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc5<128 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc5<128 * KB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_64)
            return std::make_unique<Mbc5<128 * KB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_128)
            return std::make_unique<Mbc5<128 * KB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::KB_256) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc5<256 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc5<256 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc5<256 * KB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_64)
            return std::make_unique<Mbc5<256 * KB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_128)
            return std::make_unique<Mbc5<256 * KB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::KB_512) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc5<512 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc5<512 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc5<512 * KB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_64)
            return std::make_unique<Mbc5<512 * KB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_128)
            return std::make_unique<Mbc5<512 * KB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::MB_1) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc5<1 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc5<1 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc5<1 * MB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_64)
            return std::make_unique<Mbc5<1 * MB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_128)
            return std::make_unique<Mbc5<1 * MB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::MB_2) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc5<2 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc5<2 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc5<2 * MB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_64)
            return std::make_unique<Mbc5<2 * MB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_128)
            return std::make_unique<Mbc5<2 * MB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::MB_4) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc5<4 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc5<4 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc5<4 * MB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_64)
            return std::make_unique<Mbc5<4 * MB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_128)
            return std::make_unique<Mbc5<4 * MB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Rom::MB_8) {
        if (ram == Ram::NONE)
            return std::make_unique<Mbc5<8 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_8)
            return std::make_unique<Mbc5<8 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_32)
            return std::make_unique<Mbc5<8 * MB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_64)
            return std::make_unique<Mbc5<8 * MB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Ram::KB_128)
            return std::make_unique<Mbc5<8 * MB, 128 * KB, Battery>>(data.data(), data.size());
    }

    fatal("Mbc5: unexpected data in cartridge header: rom=" + std::to_string(rom) + ", ram=" + std::to_string(ram));
}
} // namespace

std::unique_ptr<ICartridge> CartridgeFactory::create(const std::string& filename) const {
    bool ok;

    const std::vector<uint8_t> data = read_file(filename, &ok);
    if (!ok)
        fatal("failed to read file");

    if (data.size() < MemoryLayout::SIZE)
        fatal("rom size is too small");

    const uint8_t mbc = data[MemoryLayout::TYPE];
    const uint8_t rom = data[MemoryLayout::ROM_SIZE];
    const uint8_t ram = data[MemoryLayout::RAM_SIZE];

    using namespace Mbc;

    switch (mbc) {
    case NO_MBC:
        return createNoMbc(data, rom, ram);
    case MBC1:
    case MBC1_RAM:
        return createMbc1<NoBattery>(data, rom, ram);
    case MBC1_RAM_BATTERY:
        return createMbc1<Battery>(data, rom, ram);
    case MBC2:
        return createMbc2<NoBattery>(data, rom, ram);
    case MBC2_BATTERY:
        return createMbc2<Battery>(data, rom, ram);
    case MBC3_TIMER_BATTERY:
    case MBC3_TIMER_RAM_BATTERY:
        return createMbc3<Battery, Timer>(data, rom, ram);
    case MBC3:
    case MBC3_RAM:
        return createMbc3<NoBattery, NoTimer>(data, rom, ram);
    case MBC3_RAM_BATTERY:
        return createMbc3<Battery, NoTimer>(data, rom, ram);
    case MBC5:
    case MBC5_RAM:
    case MBC5_RUMBLE:
    case MBC5_RUMBLE_RAM:
        return createMbc5<NoBattery>(data, rom, ram);
    case MBC5_RAM_BATTERY:
    case MBC5_RUMBLE_RAM_BATTERY:
        return createMbc5<Battery>(data, rom, ram);
    default:
        fatal("unknown MBC type: 0x" + hex(mbc));
    }
}
