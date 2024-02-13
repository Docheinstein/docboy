#include "factory.h"
#include "docboy/cartridge/mbc1/mbc1.h"
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
} // namespace

std::unique_ptr<ICartridge> CartridgeFactory::create(const std::string& filename) const {
    bool ok;

    const std::vector<uint8_t> data = read_file(filename, &ok);
    if (!ok)
        fatal("failed to read file");

    if (data.size() < Specs::Cartridge::Header::MemoryLayout::SIZE)
        fatal("rom size is too small");

    const uint8_t mbc = data[Specs::Cartridge::Header::MemoryLayout::TYPE];
    const uint8_t rom = data[Specs::Cartridge::Header::MemoryLayout::ROM_SIZE];
    const uint8_t ram = data[Specs::Cartridge::Header::MemoryLayout::RAM_SIZE];

    switch (mbc) {
    case 0x00:
        return createNoMbc(data, rom, ram);
    case 0x01:
    case 0x02:
        return createMbc1<NoBattery>(data, rom, ram);
    case 0x03:
        return createMbc1<Battery>(data, rom, ram);
    case 0x0F:
    case 0x10:
        return createMbc3<Battery, Timer>(data, rom, ram);
    case 0x11:
    case 0x12:
        return createMbc3<NoBattery, NoTimer>(data, rom, ram);
    case 0x13:
        return createMbc3<Battery, NoTimer>(data, rom, ram);
    case 0x19:
    case 0x1A:
    case 0x1C:
    case 0x1D:
        return createMbc5<NoBattery>(data, rom, ram);
    case 0x1B:
    case 0x1E:
        return createMbc5<Battery>(data, rom, ram);
    default:
        fatal("unknown MBC type: 0x" + hex(mbc));
    }
}

std::unique_ptr<ICartridge> CartridgeFactory::createNoMbc(const std::vector<uint8_t>& data, uint8_t rom,
                                                          uint8_t ram) const {
    if (rom == Specs::Cartridge::Header::Rom::KB_32) {
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<NoMbc<8 * KB>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<NoMbc<0>>(data.data(), data.size());
    }

    fatal("NoMbc: unexpected data in cartridge header: rom=" + std::to_string(rom) + ", ram=" + std::to_string(ram));
}

template <bool Battery>
std::unique_ptr<ICartridge> CartridgeFactory::createMbc1(const std::vector<uint8_t>& data, uint8_t rom,
                                                         uint8_t ram) const {
    if (rom == Specs::Cartridge::Header::Rom::KB_32) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc1<32 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc1<32 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc1<32 * KB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_64) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc1<64 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc1<64 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc1<64 * KB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_128) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc1<128 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc1<128 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc1<128 * KB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_256) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc1<256 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc1<256 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc1<256 * KB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_512) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc1<512 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc1<512 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc1<512 * KB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::MB_1) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc1<1 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc1<1 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc1<1 * MB, 32 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::MB_2) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc1<2 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc1<2 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc1<2 * MB, 32 * KB, Battery>>(data.data(), data.size());
    }

    fatal("Mbc1: unexpected data in cartridge header: rom=" + std::to_string(rom) + ", ram=" + std::to_string(ram));
}

template <bool Battery, bool Timer>
std::unique_ptr<ICartridge> CartridgeFactory::createMbc3(const std::vector<uint8_t>& data, uint8_t rom,
                                                         uint8_t ram) const {
    if (rom == Specs::Cartridge::Header::Rom::KB_32) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc3<32 * KB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc3<32 * KB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc3<32 * KB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_64) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc3<64 * KB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc3<64 * KB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc3<64 * KB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_128) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc3<128 * KB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc3<128 * KB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc3<128 * KB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_256) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc3<256 * KB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc3<256 * KB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc3<256 * KB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_512) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc3<512 * KB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc3<512 * KB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc3<512 * KB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::MB_1) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc3<1 * MB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc3<1 * MB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc3<1 * MB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::MB_2) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc3<2 * MB, 0, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc3<2 * MB, 8 * KB, Battery, Timer>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc3<2 * MB, 32 * KB, Battery, Timer>>(data.data(), data.size());
    }

    fatal("Mbc3: unexpected data in cartridge header: rom=" + std::to_string(rom) + ", ram=" + std::to_string(ram));
}

template <bool Battery>
std::unique_ptr<ICartridge> CartridgeFactory::createMbc5(const std::vector<uint8_t>& data, uint8_t rom,
                                                         uint8_t ram) const {
    if (rom == Specs::Cartridge::Header::Rom::KB_32) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc5<32 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc5<32 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc5<32 * KB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_64)
            return std::make_unique<Mbc5<32 * KB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_128)
            return std::make_unique<Mbc5<32 * KB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_64) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc5<64 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc5<64 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc5<64 * KB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_64)
            return std::make_unique<Mbc5<64 * KB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_128)
            return std::make_unique<Mbc5<64 * KB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_128) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc5<128 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc5<128 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc5<128 * KB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_64)
            return std::make_unique<Mbc5<128 * KB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_128)
            return std::make_unique<Mbc5<128 * KB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_256) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc5<256 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc5<256 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc5<256 * KB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_64)
            return std::make_unique<Mbc5<256 * KB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_128)
            return std::make_unique<Mbc5<256 * KB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::KB_512) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc5<512 * KB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc5<512 * KB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc5<512 * KB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_64)
            return std::make_unique<Mbc5<512 * KB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc5<512 * KB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::MB_1) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc5<1 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc5<1 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc5<1 * MB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_64)
            return std::make_unique<Mbc5<1 * MB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_128)
            return std::make_unique<Mbc5<1 * MB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::MB_2) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc5<2 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc5<2 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc5<2 * MB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_64)
            return std::make_unique<Mbc5<2 * MB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_128)
            return std::make_unique<Mbc5<2 * MB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::MB_4) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc5<4 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc5<4 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc5<4 * MB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_64)
            return std::make_unique<Mbc5<4 * MB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_128)
            return std::make_unique<Mbc5<4 * MB, 128 * KB, Battery>>(data.data(), data.size());
    } else if (rom == Specs::Cartridge::Header::Rom::MB_8) {
        if (ram == Specs::Cartridge::Header::Ram::NONE)
            return std::make_unique<Mbc5<8 * MB, 0, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_8)
            return std::make_unique<Mbc5<8 * MB, 8 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_32)
            return std::make_unique<Mbc5<8 * MB, 32 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_64)
            return std::make_unique<Mbc5<8 * MB, 64 * KB, Battery>>(data.data(), data.size());
        if (ram == Specs::Cartridge::Header::Ram::KB_128)
            return std::make_unique<Mbc5<8 * MB, 128 * KB, Battery>>(data.data(), data.size());
    }

    fatal("Mbc5: unexpected data in cartridge header: rom=" + std::to_string(rom) + ", ram=" + std::to_string(ram));
}
