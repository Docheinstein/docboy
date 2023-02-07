
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
}


TEST_CASE("CPU", "[cpu]") {
    class FakeBus : public IBus {
    public:
        FakeBus() {

        }
        ~FakeBus() override = default;

        uint8_t read(uint16_t addr) override {
            io++;
            uint8_t b = 0;
            if (!data.empty()) {
                b = data.front();
                data.pop();
            }
            DEBUG(1) << "FakeBus:read(" << addr << ") -> " << (uint16_t) b <<  " [FakeBus.size() = " << data.size() << "]";
            return b;
        }

        void write(uint16_t addr, uint8_t value) override {
            io++;
        }

        [[nodiscard]] unsigned long long ioCount() const {
            return io;
        }

        void clear() {
            while (!data.empty())
                data.pop();
        }

        void feed(uint8_t b) {
            data.push(b);
        }

    private:
        std::queue<uint8_t> data;
        unsigned long long io;
    };

    FakeBus fakeBus;
    CPU cpu(fakeBus);

    auto instr = GENERATE(range(0, 0xFF));
    auto info = INSTRUCTIONS[instr];
    if (!info.duration.min)
        return;

//    SECTION("instruction implemented") {
//        fakeBus.feed(instr);
//        cpu.tick(); // fetch
//        REQUIRE_NOTHROW(cpu.tick());
//    }

    SECTION("correct duration") {
        fakeBus.feed(instr); // feed with instruction
        for (int i = 0; i < 10; i++)
            fakeBus.feed(instr + 1); // feed with something else != instr
        cpu.tick(); // fetch

        uint8_t op = cpu.getCurrentInstructionOpcode();

        try {
            for (int m = 0; m < info.duration.min - 1; m++) {
                cpu.tick();
                REQUIRE((uint16_t) op == (uint16_t) cpu.getCurrentInstructionOpcode());
            }
            cpu.tick();
            // TODO: ok for jmp/halt/... ?
            REQUIRE((uint16_t) op != (uint16_t) cpu.getCurrentInstructionOpcode());
        } catch (const CPU::InstructionNotImplementedException &e) {}
    }


    SECTION("no more than one read/write per m-cycle") {
        fakeBus.feed(instr);
        cpu.tick();

        try {
            for (int m = 0; m < info.duration.min; m++) {
                auto ioCountBefore = fakeBus.ioCount();
                cpu.tick();
                auto ioCountAfter = fakeBus.ioCount();
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