
#include "catch2/catch_session.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "catch2/generators/catch_generators_range.hpp"
#include <catch2/catch_test_macros.hpp>
#include "utils/binutils.h"
#include "utils/log.h"
#include "utils/fileutils.h"
#include "core/core.h"
#include "core/helpers.h"
#include "core/cartridge/cartridgefactory.h"
#include "core/debugger/frontend.h"
#include "core/serial/port.h"
#include "core/serial/endpoints/buffer.h"
#include "core/bus/bus.h"
#include "core/gameboy.h"
#include "core/lcd/framebufferlcd.h"
#include <queue>
#include <algorithm>
#include <cstring>

#undef SECTION
#undef DYNAMIC_SECTION
#define STATIC_SECTION(name) INTERNAL_CATCH_SECTION(name)
#define DYNAMIC_SECTION(name, param) INTERNAL_CATCH_DYNAMIC_SECTION(name << " (" << param << ")")
#define SECTION_PICKER(_1, _2, FUNC, ...) FUNC
#define SECTION(...) SECTION_PICKER( \
    __VA_ARGS__, \
    DYNAMIC_SECTION, \
    STATIC_SECTION \
)(__VA_ARGS__)


#define GENERATE_TABLE_1(t1, data) GENERATE(table<t1> data)
#define GENERATE_TABLE_2(t1, t2, data) GENERATE(table<t1, t2> data)
#define GENERATE_TABLE_3(t1, t2, t3, data) GENERATE(table<t1, t2, t3> data)

#define GENERATE_TABLE_PICKER(_1, _2, _3, _4, FUNC, ...) FUNC
#define GENERATE_TABLE(...) GENERATE_TABLE_PICKER( \
    __VA_ARGS__, \
    GENERATE_TABLE_3, \
    GENERATE_TABLE_2, \
    GENERATE_TABLE_1 \
)(__VA_ARGS__)

static std::string run_and_get_serial(const std::string &rom, uint64_t ticks) {
    class SerialString : public SerialEndpoint {
    public:
        uint8_t serialRead() override {
            return 0xFF;
        }

        void serialWrite(uint8_t b) override {
            data += (char) b;
        }

        std::string data;
    };

    SerialString serialString;

    GameBoy gb = GameBoy::Builder()
        .setFrequency(Clock::MAX_FREQUENCY)
        .build();

    Core core(gb);

    std::shared_ptr<SerialLink> serial = std::make_shared<SerialLink>();
    serial->plug1.attach(&serialString);
    core.attachSerialLink(serial->plug2);

    core.loadROM(rom);

    for (uint64_t i = 0; i < ticks; i++)
        core.tick();

    return serialString.data;
}


static std::string run_and_expect_string_over_serial(
        const std::string &rom, const std::string &expected, uint64_t maxticks) {
    class SerialString : public SerialEndpoint {
    public:
        uint8_t serialRead() override {
            return 0xFF;
        }

        void serialWrite(uint8_t b) override {
            data += (char) b;
        }

        std::string data;
    };

    SerialString serialString;

    GameBoy gb = GameBoy::Builder()
        .setFrequency(Clock::MAX_FREQUENCY)
        .build();

    DebuggableCore core(gb);

    std::shared_ptr<SerialLink> serial = std::make_shared<SerialLink>();
    serial->plug1.attach(&serialString);
    core.attachSerialLink(serial->plug2);

    core.loadROM(rom);

    while (core.getTicks() < maxticks) {
        for (uint64_t i = 0; i < 100000; i++)
            core.tick();
        if (serialString.data.size() >= expected.size())
            break;
    }

    return serialString.data;
}

static std::vector<uint32_t> run_and_get_framebuffer(const std::string &rom, uint64_t ticks) {
    std::shared_ptr<IFrameBufferLCD> lcd = std::make_shared<FrameBufferLCD>();

    GameBoy gb = GameBoy::Builder()
            .setFrequency(Clock::MAX_FREQUENCY)
            .setLCD(lcd)
            .build();

    Core core(gb);
    core.loadROM(rom);

    for (uint64_t i = 0; i < ticks; i++)
        core.tick();

    uint32_t *framebuffer = lcd->getFrameBuffer();
    std::vector<uint32_t> out;
    size_t length = Specs::Display::WIDTH * Specs::Display::HEIGHT * sizeof(uint32_t);
    out.resize(length / sizeof(uint32_t));
    memcpy(out.data(), framebuffer, length);

    return out;
}

TEST_CASE("binutils", "[binutils]") {
    SECTION("get and set bit") {
        uint8_t B = 0;
        REQUIRE(get_bit<7>(B) == 0);
        REQUIRE(get_bit<6>(B) == 0);
        REQUIRE(get_bit<5>(B) == 0);
        REQUIRE(get_bit<4>(B) == 0);
        REQUIRE(get_bit<3>(B) == 0);
        REQUIRE(get_bit<2>(B) == 0);
        REQUIRE(get_bit<1>(B) == 0);
        REQUIRE(get_bit<0>(B) == 0);

        B = ~B;

        REQUIRE(get_bit<7>(B) == 1);
        REQUIRE(get_bit<6>(B) == 1);
        REQUIRE(get_bit<5>(B) == 1);
        REQUIRE(get_bit<4>(B) == 1);
        REQUIRE(get_bit<3>(B) == 1);
        REQUIRE(get_bit<2>(B) == 1);
        REQUIRE(get_bit<1>(B) == 1);
        REQUIRE(get_bit<0>(B) == 1);

        set_bit<0>(B, false);
        REQUIRE(get_bit<0>(B) == 0);

        REQUIRE(B == 0xFE);
    }

    SECTION("get and set byte") {
        uint16_t AF = 0;
        REQUIRE(get_byte<0>(AF) == 0);
        REQUIRE(get_byte<1>(AF) == 0);

        set_byte<0>(AF, 3);
        REQUIRE(AF == 3);

        set_byte<0>(AF, 0);
        set_byte<1>(AF, 1);
        REQUIRE(get_byte<0>(AF) == 0);
        REQUIRE(get_byte<1>(AF) == 1);
        REQUIRE(AF == 256);

        set_byte<0>(AF, 3);
        set_byte<1>(AF, 4);
        REQUIRE(AF == 0x0403);
    }

    SECTION("get and set nibble") {
        uint16_t X = 0xF2;
        REQUIRE(get_nibble<0>(X) == 0x2);
        REQUIRE(get_nibble<1>(X) == 0xF);

        set_nibble<0>(X, 3);
        REQUIRE(X == 0xF3);

        set_nibble<1>(X, 0);
        REQUIRE(X == 0x3);
        REQUIRE(get_nibble<0>(X) == 0x3);
        REQUIRE(get_nibble<1>(X) == 0x0);
    }

    SECTION("concat bytes") {
        uint8_t A = 0x03;
        uint8_t B = 0x01;
        REQUIRE(concat_bytes(A, B) == 0x0301);
        REQUIRE(concat_bytes(B, A) == 0x0103);
    }

    SECTION("bitmask_on") {
        REQUIRE(bitmask_on<0> == 0b00000000);
        REQUIRE(bitmask_on<1> == 0b00000001);
        REQUIRE(bitmask_on<2> == 0b00000011);
        REQUIRE(bitmask_on<3> == 0b00000111);
        REQUIRE(bitmask_on<4> == 0b00001111);
        REQUIRE(bitmask_on<5> == 0b00011111);
        REQUIRE(bitmask_on<6> == 0b00111111);
        REQUIRE(bitmask_on<7> == 0b01111111);
        REQUIRE(bitmask_on<8> == 0b11111111);
    }
    SECTION("bitmask_off") {
        REQUIRE((uint8_t) bitmask_off<0> == 0b11111111);
        REQUIRE((uint8_t) bitmask_off<1> == 0b11111110);
        REQUIRE((uint8_t) bitmask_off<2> == 0b11111100);
        REQUIRE((uint8_t) bitmask_off<3> == 0b11111000);
        REQUIRE((uint8_t) bitmask_off<4> == 0b11110000);
        REQUIRE((uint8_t) bitmask_off<5> == 0b11100000);
        REQUIRE((uint8_t) bitmask_off<6> == 0b11000000);
        REQUIRE((uint8_t) bitmask_off<7> == 0b10000000);
        REQUIRE((uint8_t) bitmask_off<8> == 0b00000000);
    }

    SECTION("bit") {
        REQUIRE(bit<0> == (1 << 0));
        REQUIRE(bit<1> == (1 << 1));
        REQUIRE(bit<2> == (1 << 2));
        REQUIRE(bit<3> == (1 << 3));
        REQUIRE(bit<4> == (1 << 4));
        REQUIRE(bit<5> == (1 << 5));
        REQUIRE(bit<6> == (1 << 6));
        REQUIRE(bit<7> == (1 << 7));
        REQUIRE(bit<8> == (1 << 8));
        REQUIRE(bit<9> == (1 << 9));
        REQUIRE(bit<10> == (1 << 10));
        REQUIRE(bit<11> == (1 << 11));
        REQUIRE(bit<12> == (1 << 12));
        REQUIRE(bit<13> == (1 << 13));
        REQUIRE(bit<14> == (1 << 14));
        REQUIRE(bit<15> == (1 << 15));
        REQUIRE(bit<16> == (1 << 16));
    }

    SECTION("carry bit") {
        int8_t s1;
        uint8_t u1, u2;

        u1 = 3;
        u2 = 1;
        REQUIRE(sum_get_carry_bit<0>(u1, u2));
        REQUIRE(sum_get_carry_bit<1>(u1, u2));
        REQUIRE_FALSE(sum_get_carry_bit<2>(u1, u2));

        auto [result, c0, c1] = sum_carry<0, 1>(u1, u2);
        REQUIRE(result == 4);
        REQUIRE(c0);
        REQUIRE(c1);

        uint16_t uu1 = 0xB;
        uint16_t uu2 = 0x7;
        REQUIRE(sum_get_carry_bit<3>(uu1, uu2));
        REQUIRE_FALSE(sum_get_carry_bit<4>(uu1, uu2));

        uu1 = 0xFFFF;
        uu2 = 0xFFFF;
        REQUIRE(sum_get_carry_bit<7>(uu1, uu2));

        u1 = 0xFF;
        auto [result2, _1] = sum_carry<3>(u1, (uint8_t) 1);
        REQUIRE(result2 == 0);

//        u1 = 4;
//        auto [result3, _2] = sum_carry<3>(u1, -1);
//        REQUIRE(result3 == 3);

        u1 = 0x10;
        s1 = 1;
        auto [result6, h2] = sub_borrow<3>(u1, 1);
        REQUIRE(result6 == 0x0F);
        REQUIRE(h2);

        /*
        u1 = 0;
        s1 = -1;
        auto [result4, b0] = sum_carry<3>(u1, s1);
        REQUIRE(result4 == 0xFF);
        REQUIRE(b0);

        u1 = 1;
        s1 = -1;
        auto [result5, b1] = sum_carry<3>(u1, s1);
        REQUIRE(result5 == 0);
        REQUIRE_FALSE(b1);




        uu1 = 0xFFFF;
        uu2 = 0xFFFF;
        auto [_, b3] = sum_carry<15>(uu1, uu2);
        REQUIRE(b3);

        uu1 = 0x000F;
        s1 = 0x01;
        auto [result7, h, c] = sum_carry<3, 7>(uu1, s1);
        REQUIRE(result7 == 0x0010);
        REQUIRE(h);

        */

        uu1 = 0x0000;
        s1 = -1;
        auto [result8, h3, c3] = sum_carry<3, 7>(uu1, s1);
        REQUIRE(result8 == 0xFFFF);
        REQUIRE_FALSE(h3);
        REQUIRE_FALSE(c3);
    }
}

TEST_CASE("cartridge", "[cartridge]") {
    auto [rom, title] = GENERATE_TABLE(
        std::string,
        std::string,
        ({
            { "tests/roms/games/tetris.gb", "TETRIS" },
            { "tests/roms/games/alleyway.gb", "ALLEY WAY" }
        })
    );

    auto c = CartridgeFactory::makeCartridge(rom);

    SECTION("cartridge loaded", rom) {
        REQUIRE(c);
    }

    SECTION("cartridge header valid", rom) {
        Cartridge::Header h = c->header();
        REQUIRE(h.title == title);
        REQUIRE(h.isNintendoLogoValid());
        REQUIRE(h.isHeaderChecksumValid());
    }
}

TEST_CASE("cpu", "[cpu]") {
    SECTION("instruction basic requirements") {
        class FakeBus : public IBus {
        public:
            struct Access {
                enum Type {
                    Read,
                    Write
                } type;
                uint16_t addr;
            };

            FakeBus() = default;

            ~FakeBus() override = default;

            [[nodiscard]] uint8_t read(uint16_t addr) const override {
                if (addr >= MemoryMap::IO::START)
                    return 0; // don't count interrupts, timers, ...

                accesses.push_back({Access::Type::Read, addr});
                uint8_t b = 0;
                if (!data.empty()) {
                    b = data.front();
                    data.pop();
                }
                return b;
            }

            void write(uint16_t addr, uint8_t value) override {
                if (addr == MemoryMap::IE || addr == Registers::Interrupts::IF)
                    return;
                accesses.push_back({Access::Type::Write, addr});
            }

            void attachCartridge(IMemory *cartridge) override {}
            void detachCartridge() override {}

            [[nodiscard]] unsigned long long getReadWriteCount() const {
                return accesses.size();
            }

            [[nodiscard]] unsigned long long getReadCount() const {
                return std::count_if(accesses.begin(), accesses.end(), [](const Access &a) {
                    return a.type == Access::Read;
                });
            }

            [[nodiscard]] unsigned long long getWriteCount() const {
                return std::count_if(accesses.begin(), accesses.end(), [](const Access &a) {
                    return a.type == Access::Write;
                });
            }

            [[nodiscard]] const std::vector<Access> &getAccesses() const {
                return accesses;
            }

            void clear() {
                while (!data.empty())
                    data.pop();
            }

            void feed(uint8_t b) {
                data.push(b);
            }

        private:
            mutable std::vector<Access> accesses;
            mutable std::queue<uint8_t> data;
        };

        bool cb = GENERATE(false, true);
        uint8_t instr = GENERATE(range(0, 0xFF));

        std::string instr_name = instruction_mnemonic(instr, cb);

        auto length = instruction_length(instr, cb);
        auto [duration_min, duration_max] = instruction_duration(instr, cb);

        if (!duration_min)
            return;
        if (!cb && instr == 0x76 /* HALT */)
            return;

        FakeBus fakeBus;
        SerialPort serialPort(fakeBus);
        DebuggableCPU cpu(fakeBus, serialPort);

        auto setupInstruction = [&fakeBus, &cpu, cb, instr]() {
            if (cb)
                fakeBus.feed(0xCB);
            fakeBus.feed(instr); // feed with instruction
            for (int i = 0; i < 10; i++)
                fakeBus.feed((instr + 1) * 3); // feed with something else != instr
            cpu.tick(); // fetch
            if (cb)
                cpu.tick(); // fetch
        };

        auto getInstructionLength = [&fakeBus]() {
            auto accesses = fakeBus.getAccesses();
            unsigned long long length = 0;

            auto lastRead = std::find_if(accesses.rend(), accesses.rbegin(), [](const FakeBus::Access &a) {
                return a.type == FakeBus::Access::Read;
            });
            std::optional<FakeBus::Access> lastReadAddress;
            // count sequential reads but skip the last one (fetch)
            for (auto it = accesses.begin(); it != (lastRead.base() - 1); it++) {
                auto a = *it;
                if (a.type != FakeBus::Access::Read)
                    continue;
                if (!lastReadAddress) {
                    ++length;
                    lastReadAddress = a;
                    continue;
                }
                if (lastReadAddress->addr + 1 == a.addr) {
                    ++length;
                    lastReadAddress = a;
                }
            }

            return length;
        };


        SECTION("instruction implemented", instr_name) {
            setupInstruction();
            REQUIRE_NOTHROW(cpu.tick());
        }

        SECTION("instruction duration", instr_name) {
            setupInstruction();

            try {
                uint8_t duration = cb ? 1 : 0;
                do {
                    cpu.tick();
                    duration++;
                } while (cpu.getCurrentInstruction().microop != 0);
                REQUIRE(duration_min <= duration);
                REQUIRE(duration <= duration_max);
            } catch (const std::runtime_error &e) {}
        }

        SECTION("instruction length", instr_name) {
            setupInstruction();
            try {
                do {
                    cpu.tick();
                } while (cpu.getCurrentInstruction().microop != 0);
                REQUIRE(length == getInstructionLength());
            } catch (const std::runtime_error &e) {}
        }

        SECTION("no more than one memory read/write per m-cycle", instr_name) {
            setupInstruction();
            try {
                for (int m = 0; m < duration_min; m++) {
                    auto ioCountBefore = fakeBus.getReadWriteCount();
                    cpu.tick();
                    auto ioCountAfter = fakeBus.getReadWriteCount();
                    REQUIRE(ioCountAfter - ioCountBefore <= 1);
                }
            } catch (const std::runtime_error &e) {}
        }
    }
}

TEST_CASE("serial", "[serial]") {
    SECTION("serial link and serial buffer") {
        class SerialStringSource : public SerialEndpoint {
        public:
            explicit SerialStringSource(const std::string &s) : cursor() {
                for (auto c : s)
                    data.push_back(c);
            }

            uint8_t serialRead() override {
                if (!hasData())
                    return 0xFF;
                return data[cursor++];
            }

            void serialWrite(uint8_t) override {}

            [[nodiscard]] const std::vector<uint8_t> &getData() const {
                return data;
            }

            [[nodiscard]] bool hasData() const {
                return cursor < data.size();
            }

        private:
            std::vector<uint8_t> data;
            size_t cursor;
        };

        std::string s = "Hello this is a test\nThis is sparta!";
        SerialBuffer receiver;
        SerialStringSource sender(s);
        SerialLink serialLink;
        serialLink.plug1.attach(&sender);
        serialLink.plug2.attach(&receiver);
        while (sender.hasData())
            serialLink.tick();
        REQUIRE(receiver.getData() == sender.getData());
    }
}

TEST_CASE("blargg_cpu", "[.][cpu][core][timer][interrupt][blargg]") {
    std::string testname = GENERATE(
        "01-special",
        "02-interrupts",
        "03-op sp,hl",
        "04-op r,imm",
        "05-op rp",
        "06-ld r,r",
        "07-jr,jp,call,ret,rst",
        "08-misc instrs",
        "09-op r,r",
        "10-bit ops",
        "11-op a,(hl)"
    );

    SECTION("cpu_instrs", testname) {
        std::string rom = "tests/roms/tests/blargg/" + testname + ".gb";
        std::string expectedSerial = testname + "\n\n\nPassed\n";
        std::string serial = run_and_expect_string_over_serial(rom, expectedSerial, 150'000'000);
        REQUIRE(serial == expectedSerial);
    }
}

TEST_CASE("blargg_ppu", "[.][ppu][blargg]") {
    auto [rom, screenshot, ticks] = GENERATE_TABLE(
        std::string,
        std::string,
        uint64_t,
        ({
            { "tests/roms/tests/blargg/06-ld r,r.gb", "tests/screenshots/blargg/06-ld r,r.dat", 6358441 },
        })
    );

    SECTION("cpu_instrs", rom) {
        std::vector<uint32_t> expectedFramebuffer = read_file<uint32_t>(screenshot);
        std::vector<uint32_t> framebuffer = run_and_get_framebuffer(rom, ticks);
        REQUIRE(framebuffer == expectedFramebuffer);
    }
}

int main(int argc, char* argv[]) {
    Catch::Session session;
    session.applyCommandLine(argc, argv);
    return session.run();
}