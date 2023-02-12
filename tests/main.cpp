
#include "catch2/catch_session.hpp"
#include "core/gible.h"
#include "catch2/generators/catch_generators.hpp"
#include "catch2/generators/catch_generators_range.hpp"
#include <catch2/catch_test_macros.hpp>
#include "core/cartridge.h"
#include "utils/binutils.h"
#include "core/definitions.h"
#include "log/log.h"
#include <queue>
#include <iostream>
#include <type_traits>
#include <algorithm>

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
    }

    SECTION("concat bytes") {
        uint8_t A = 0x03;
        uint8_t B = 0x01;
        REQUIRE(concat_bytes(A, B) == 0x0301);
        REQUIRE(concat_bytes(B, A) == 0x0103);
    }

    SECTION("bitmask") {
        REQUIRE(bitmask<0>() == 0x0);
        REQUIRE(bitmask<1>() == 0x1);
        REQUIRE(bitmask<2>() == 0x3);
        REQUIRE(bitmask<3>() == 0x7);
        REQUIRE(bitmask<4>() == 0xF);
        REQUIRE(bitmask<5>() == 0x1F);
        REQUIRE(bitmask<6>() == 0x3F);
        REQUIRE(bitmask<7>() == 0x7F);
        REQUIRE(bitmask<8>() == 0xFF);
        REQUIRE(bitmask<9>() == 0x1FF);
        REQUIRE(bitmask<10>() == 0x3FF);
        REQUIRE(bitmask<11>() == 0x7FF);
        REQUIRE(bitmask<12>() == 0xFFF);
        REQUIRE(bitmask<13>() == 0x1FFF);
        REQUIRE(bitmask<14>() == 0x3FFF);
        REQUIRE(bitmask<15>() == 0x7FFF);
        REQUIRE(bitmask<16>() == 0xFFFF);
    }

    SECTION("carry bit") {
        uint8_t u1 = 3;
        uint8_t u2 = 1;
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
    }
}


TEST_CASE("CPU", "[cpu]") {
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

        uint8_t read(uint16_t addr) override {
            accesses.push_back({Access::Type::Read, addr});
            uint8_t b = 0;
            if (!data.empty()) {
                b = data.front();
                data.pop();
            }
            return b;
        }

        void write(uint16_t addr, uint8_t value) override {
            accesses.push_back({Access::Type::Write, addr});
        }

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

        [[nodiscard]] const std::vector<Access> & getAccesses() const {
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
        std::vector<Access> accesses;
        std::queue<uint8_t> data;
    };

    bool cb = GENERATE(false, true);
    uint8_t instr = GENERATE(range(0, 0xFF));

    InstructionInfo info = cb ? INSTRUCTIONS_CB[instr] : INSTRUCTIONS[instr];
    if (!info.duration.min)
        return;
    // TODO: better handling of special cases
    if (!cb && instr == 0x76 /* HALT */)
        return;

    auto instr_name = (cb ? hex((uint8_t) 0xCB) +  " " : "") + hex(instr) + " " + info.mnemonic;

    FakeBus fakeBus;
    CPU cpu(fakeBus);

    auto setupInstruction = [&fakeBus, &cpu, cb, instr]() {
        if (cb)
            fakeBus.feed(0xCB);
        fakeBus.feed(instr); // feed with instruction
        for (int i = 0; i < 10; i++)
            fakeBus.feed((instr + 1)* 3); // feed with something else != instr
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


//    SECTION("instruction implemented", instr_name) {
//        setupInstruction();
//        REQUIRE_NOTHROW(cpu.tick());
//    }

    SECTION("instruction duration", instr_name) {
        setupInstruction();
        uint8_t op = cpu.getCurrentInstructionOpcode();

        try {
            uint8_t duration = cb;
            do {
                cpu.tick();
                duration++;
            } while (op == cpu.getCurrentInstructionOpcode());
            REQUIRE(info.duration.min <= duration);
            REQUIRE(duration <= info.duration.max);
        } catch (const CPU::InstructionNotImplementedException &e) {}
    }

    SECTION("instruction length", instr_name) {
        setupInstruction();
        try {
            uint8_t op = cpu.getCurrentInstructionOpcode();
            do {
                cpu.tick();
            } while (op == cpu.getCurrentInstructionOpcode());
            REQUIRE(info.length == getInstructionLength());
        } catch (const CPU::InstructionNotImplementedException &e) {}
    }

    SECTION("no more than one memory read/write per m-cycle", instr_name) {
        setupInstruction();
        try {
            for (int m = 0; m < info.duration.min; m++) {
                auto ioCountBefore = fakeBus.getReadWriteCount();
                cpu.tick();
                auto ioCountAfter = fakeBus.getReadWriteCount();
                REQUIRE(ioCountAfter - ioCountBefore <= 1);
            }
        } catch (const CPU::InstructionNotImplementedException &e) {}
    }
}

TEST_CASE("Cartridge", "[cartridge]") {
    auto [rom, title] = GENERATE_TABLE(
        std::string,
        std::string,
        ({
            { "tests/roms/tetris.gb", "TETRIS" },
            { "tests/roms/alleyway.gb", "ALLEY WAY" },
            { "tests/roms/pokemon-red.gb", "POKEMON RED" },
        })
    );

    auto c = Cartridge::fromFile(rom);

    SECTION("cartridge loaded") {
        REQUIRE(c);
    }

    SECTION("cartridge header valid") {
        Cartridge::Header h = c->header();
        REQUIRE(h.title == title);
        REQUIRE(h.isNintendoLogoValid());
        REQUIRE(h.isHeaderChecksumValid());
    }
}

int main(int argc, char* argv[]) {
    Catch::Session session;
    session.applyCommandLine(argc, argv);
    return session.run();
}