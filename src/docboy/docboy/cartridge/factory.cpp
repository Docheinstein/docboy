#include "docboy/cartridge/factory.h"

#include "docboy/cartridge/cartridge.h"
#include "docboy/cartridge/huc1/huc1.h"
#include "docboy/cartridge/huc3/huc3.h"
#include "docboy/cartridge/mbc1/mbc1.h"
#include "docboy/cartridge/mbc1/mbc1m.h"
#include "docboy/cartridge/mbc2/mbc2.h"
#include "docboy/cartridge/mbc3/mbc3.h"
#include "docboy/cartridge/mbc5/mbc5.h"
#include "docboy/cartridge/nombc/nombc.h"
#include "docboy/cartridge/romram/romram.h"
#include "docboy/common/specs.h"

#include "utils/exceptions.h"
#include "utils/formatters.h"
#include "utils/io.h"

namespace {
constexpr bool Battery = true;
constexpr bool NoBattery = false;

constexpr bool Timer = true;
constexpr bool NoTimer = false;

constexpr bool Rumble = true;
constexpr bool NoRumble = false;

constexpr uint32_t KB = 1 << 10;
constexpr uint32_t MB = 1 << 20;

using namespace Specs::Cartridge::Header;

namespace Info {
    namespace NoMbc {
        constexpr uint16_t Rom = bits<Rom::KB_32>();
        constexpr uint16_t Ram = bits<Ram::NONE, Ram::KB_8>();
    } // namespace NoMbc
    namespace Mbc1 {
        constexpr uint16_t Rom =
            bits<Rom::KB_32, Rom::KB_64, Rom::KB_128, Rom::KB_256, Rom::KB_512, Rom::MB_1, Rom::MB_2>();
        constexpr uint16_t Ram = bits<Ram::NONE, Ram::KB_8, Ram::KB_32>();
    } // namespace Mbc1
    namespace Mbc2 {
        constexpr uint16_t Rom = bits<Rom::KB_32, Rom::KB_64, Rom::KB_128, Rom::KB_256>();
        constexpr uint16_t Ram = bits<Ram::NONE>();
    } // namespace Mbc2
    namespace Mbc3 {
        constexpr uint16_t Rom =
            bits<Rom::KB_32, Rom::KB_64, Rom::KB_128, Rom::KB_256, Rom::KB_512, Rom::MB_1, Rom::MB_2>();
        constexpr uint16_t Ram = bits<Ram::NONE, Ram::KB_2, Ram::KB_8, Ram::KB_32>();
    } // namespace Mbc3
    namespace Mbc5 {
        constexpr uint16_t Rom = bits<Rom::KB_32, Rom::KB_64, Rom::KB_128, Rom::KB_256, Rom::KB_512, Rom::MB_1,
                                      Rom::MB_2, Rom::MB_4, Rom::MB_8>();
        constexpr uint16_t Ram = bits<Ram::NONE, Ram::KB_8, Ram::KB_32, Ram::KB_64, Ram::KB_128>();
    } // namespace Mbc5
    namespace RomRam {
        constexpr uint16_t Rom = bits<Rom::KB_32>();
        constexpr uint16_t Ram = bits<Ram::NONE, Ram::KB_2, Ram::KB_8, Ram::KB_32>();
    } // namespace RomRam
    namespace HuC3 {
        constexpr uint16_t Rom =
            bits<Rom::KB_32, Rom::KB_64, Rom::KB_128, Rom::KB_256, Rom::KB_512, Rom::MB_1, Rom::MB_2>();
        constexpr uint16_t Ram = bits<Ram::KB_32>();
    } // namespace HuC3
    namespace HuC1 {
        constexpr uint16_t Rom = bits<Rom::KB_32, Rom::KB_64, Rom::KB_128, Rom::KB_256, Rom::KB_512, Rom::MB_1>();
        constexpr uint16_t Ram = bits<Ram::KB_32>();
    } // namespace HuC1
} // namespace Info

template <uint32_t RomSize, uint32_t RamSize>
using NoMbc_ = NoMbc<RamSize>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc1_Battery = Mbc1<RomSize, RamSize, Battery>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc1_NoBattery = Mbc1<RomSize, RamSize, NoBattery>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc1M_Battery = Mbc1M<RomSize, RamSize, Battery>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc1M_NoBattery = Mbc1M<RomSize, RamSize, NoBattery>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc2_Battery = Mbc2<RomSize, Battery>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc2_NoBattery = Mbc2<RomSize, NoBattery>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc3_NoBattery_NoTimer = Mbc3<RomSize, RamSize, NoBattery, NoTimer>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc3_Battery_NoTimer = Mbc3<RomSize, RamSize, Battery, NoTimer>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc3_Battery_Timer = Mbc3<RomSize, RamSize, Battery, Timer>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc5_Battery_NoRumble = Mbc5<RomSize, RamSize, Battery, NoRumble>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc5_Battery_Rumble = Mbc5<RomSize, RamSize, Battery, Rumble>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc5_NoBattery_NoRumble = Mbc5<RomSize, RamSize, NoBattery, NoRumble>;

template <uint32_t RomSize, uint32_t RamSize>
using Mbc5_NoBattery_Rumble = Mbc5<RomSize, RamSize, NoBattery, Rumble>;

template <uint32_t RomSize, uint32_t RamSize>
using RomRam_Battery = RomRam<RomSize, RamSize, Battery>;

template <uint32_t RomSize, uint32_t RamSize>
using RomRam_NoBattery = RomRam<RomSize, RamSize, NoBattery>;

bool is_mbc1m(const std::vector<uint8_t>& data) {
    // MBC1M can't be distinguished from MBC1 only from the header.
    // The heuristic for distinguish it is the presence of (at least two)
    // NINTENDO LOGO at the beginning of each game's bank.
    if (data.size() < 0x100000 ||
        (data[MemoryLayout::TYPE] != Mbc::MBC1 && data[MemoryLayout::TYPE] != Mbc::MBC1_RAM &&
         data[MemoryLayout::TYPE] != Mbc::MBC1_RAM_BATTERY) ||
        data[MemoryLayout::ROM_SIZE] != Rom::MB_1) {
        return false;
    }

    // Count the occurrences of Nintendo Logo.
    uint8_t num_logo {};
    for (uint8_t i = 0; i < 4; i++) {
        num_logo +=
            memcmp(data.data() + (0x40000 * i) + MemoryLayout::LOGO::START, NINTENDO_LOGO, sizeof(NINTENDO_LOGO)) == 0;
    }

    return num_logo > 1;
}

template <template <uint32_t, uint32_t> typename T, uint32_t RomSize, uint32_t RamSize>
std::unique_ptr<ICartridge> create(const std::vector<uint8_t>& data) {
    return std::make_unique<T<RomSize, RamSize>>(data.data(), data.size());
}

template <template <uint32_t, uint32_t> typename T, uint32_t RomSize, uint16_t SupportedRamSize>
std::unique_ptr<ICartridge> create(const std::vector<uint8_t>& data, uint8_t ram) {
    if constexpr (test_bit<SupportedRamSize, Ram::NONE>()) {
        if (ram == Ram::NONE) {
            return create<T, RomSize, 0>(data);
        }
    }
    if constexpr (test_bit<SupportedRamSize, Ram::KB_2>()) {
        if (ram == Ram::KB_2) {
            return create<T, RomSize, 2 * KB>(data);
        }
    }
    if constexpr (test_bit<SupportedRamSize, Ram::KB_8>()) {
        if (ram == Ram::KB_8) {
            return create<T, RomSize, 8 * KB>(data);
        }
    }
    if constexpr (test_bit<SupportedRamSize, Ram::KB_32>()) {
        if (ram == Ram::KB_32) {
            return create<T, RomSize, 32 * KB>(data);
        }
    }
    if constexpr (test_bit<SupportedRamSize, Ram::KB_128>()) {
        if (ram == Ram::KB_128) {
            return create<T, RomSize, 128 * KB>(data);
        }
    }
    if constexpr (test_bit<SupportedRamSize, Ram::KB_64>()) {
        if (ram == Ram::KB_64) {
            return create<T, RomSize, 64 * KB>(data);
        }
    }

    return nullptr;
}

template <template <uint32_t, uint32_t> typename T, uint16_t SupportedRomSize, uint16_t SupportedRamSize>
std::unique_ptr<ICartridge> create(const std::vector<uint8_t>& data, uint8_t rom, uint8_t ram) {
    if constexpr (test_bit<SupportedRomSize, Rom::KB_32>()) {
        if (rom == Rom::KB_32) {
            return create<T, 32 * KB, SupportedRamSize>(data, ram);
        }
    }
    if constexpr (test_bit<SupportedRomSize, Rom::KB_64>()) {
        if (rom == Rom::KB_64) {
            return create<T, 64 * KB, SupportedRamSize>(data, ram);
        }
    }
    if constexpr (test_bit<SupportedRomSize, Rom::KB_128>()) {
        if (rom == Rom::KB_128) {
            return create<T, 128 * KB, SupportedRamSize>(data, ram);
        }
    }
    if constexpr (test_bit<SupportedRomSize, Rom::KB_256>()) {
        if (rom == Rom::KB_256) {
            return create<T, 256 * KB, SupportedRamSize>(data, ram);
        }
    }
    if constexpr (test_bit<SupportedRomSize, Rom::KB_512>()) {
        if (rom == Rom::KB_512) {
            return create<T, 512 * KB, SupportedRamSize>(data, ram);
        }
    }
    if constexpr (test_bit<SupportedRomSize, Rom::MB_1>()) {
        if (rom == Rom::MB_1) {
            return create<T, 1 * MB, SupportedRamSize>(data, ram);
        }
    }
    if constexpr (test_bit<SupportedRomSize, Rom::MB_2>()) {
        if (rom == Rom::MB_2) {
            return create<T, 2 * MB, SupportedRamSize>(data, ram);
        }
    }
    if constexpr (test_bit<SupportedRomSize, Rom::MB_4>()) {
        if (rom == Rom::MB_4) {
            return create<T, 4 * MB, SupportedRamSize>(data, ram);
        }
    }
    if constexpr (test_bit<SupportedRomSize, Rom::MB_4>()) {
        if (rom == Rom::MB_8) {
            return create<T, 8 * MB, SupportedRamSize>(data, ram);
        }
    }
    return nullptr;
}

std::unique_ptr<ICartridge> create(const std::vector<uint8_t>& data, uint8_t mbc, uint8_t rom, uint8_t ram) {
    switch (mbc) {
    case Mbc::NO_MBC:
        return create<NoMbc_, Info::NoMbc::Rom, Info::NoMbc::Ram>(data, rom, ram);
    case Mbc::MBC1:
    case Mbc::MBC1_RAM:
        if (is_mbc1m(data)) {
            return create<Mbc1M_NoBattery, Info::Mbc1::Rom, Info::Mbc1::Ram>(data, rom, ram);
        } else {
            return create<Mbc1_NoBattery, Info::Mbc1::Rom, Info::Mbc1::Ram>(data, rom, ram);
        }
    case Mbc::MBC1_RAM_BATTERY:
        if (is_mbc1m(data)) {
            return create<Mbc1M_Battery, Info::Mbc1::Rom, Info::Mbc1::Ram>(data, rom, ram);
        } else {
            return create<Mbc1_Battery, Info::Mbc1::Rom, Info::Mbc1::Ram>(data, rom, ram);
        }
    case Mbc::MBC2:
        return create<Mbc2_NoBattery, Info::Mbc2::Rom, Info::Mbc2::Ram>(data, rom, ram);
    case Mbc::MBC2_BATTERY:
        return create<Mbc2_Battery, Info::Mbc2::Rom, Info::Mbc2::Ram>(data, rom, ram);
    case Mbc::ROM_RAM:
        return create<RomRam_Battery, Info::RomRam::Rom, Info::RomRam::Ram>(data, rom, ram);
    case Mbc::ROM_RAM_BATTERY:
        return create<RomRam_NoBattery, Info::RomRam::Rom, Info::RomRam::Ram>(data, rom, ram);
    case Mbc::MBC3_TIMER_BATTERY:
    case Mbc::MBC3_TIMER_RAM_BATTERY:
        return create<Mbc3_Battery_Timer, Info::Mbc3::Rom, Info::Mbc3::Ram>(data, rom, ram);
    case Mbc::MBC3:
    case Mbc::MBC3_RAM:
        return create<Mbc3_NoBattery_NoTimer, Info::Mbc3::Rom, Info::Mbc3::Ram>(data, rom, ram);
    case Mbc::MBC3_RAM_BATTERY:
        return create<Mbc3_Battery_NoTimer, Info::Mbc3::Rom, Info::Mbc3::Ram>(data, rom, ram);
    case Mbc::MBC5:
    case Mbc::MBC5_RAM:
        return create<Mbc5_NoBattery_NoRumble, Info::Mbc5::Rom, Info::Mbc5::Ram>(data, rom, ram);
    case Mbc::MBC5_RUMBLE:
    case Mbc::MBC5_RUMBLE_RAM:
        return create<Mbc5_NoBattery_Rumble, Info::Mbc5::Rom, Info::Mbc5::Ram>(data, rom, ram);
    case Mbc::MBC5_RAM_BATTERY:
        return create<Mbc5_Battery_NoRumble, Info::Mbc5::Rom, Info::Mbc5::Ram>(data, rom, ram);
    case Mbc::MBC5_RUMBLE_RAM_BATTERY:
        return create<Mbc5_Battery_Rumble, Info::Mbc5::Rom, Info::Mbc5::Ram>(data, rom, ram);
    case Mbc::HUC3:
        return create<HuC3, Info::HuC3::Rom, Info::HuC3::Ram>(data, rom, ram);
    case Mbc::HUC1:
        return create<HuC1, Info::HuC1::Rom, Info::HuC1::Ram>(data, rom, ram);
    default:
        FATAL("unknown MBC type: 0x" + hex(mbc));
    }
}
} // namespace

std::unique_ptr<ICartridge> CartridgeFactory::create(const std::string& filename) {
    bool ok;

    const std::vector<uint8_t> data = read_file(filename, &ok);
    if (!ok) {
        FATAL("failed to read file '" + filename + "'");
    }

    if (data.size() < MemoryLayout::SIZE) {
        FATAL("rom size is too small");
    }

    const uint8_t mbc = data[MemoryLayout::TYPE];
    const uint8_t rom = data[MemoryLayout::ROM_SIZE];
    const uint8_t ram = data[MemoryLayout::RAM_SIZE];

    auto cartridge = ::create(data, mbc, rom, ram);

    if (!cartridge) {
        FATAL("cartridge not supported: MBC=0x" + hex(mbc) + ", ROM=0x" + hex(rom) + ", RAM=0x" + hex(ram));
    }

    return cartridge;
}
