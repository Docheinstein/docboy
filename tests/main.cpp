#include "catch2/catch_session.hpp"
#include "core/gible.h"
#include "catch2/generators/catch_generators.hpp"
#include <catch2/catch_test_macros.hpp>
#include "core/cartridge.h"
#include "utils/binutils.h"
#include <iostream>

#define TESTS_PATH "../tests"

#define DATA1(t1, v1, data) \
    auto [v1] = GENERATE(table<t1> data)
#define DATA2(t1, v1, t2, v2, data) \
    auto [v1, v2] = GENERATE(table<t1, t2> data)
#define DATA3(t1, v1, t2, v2, t3, v3, data) \
    auto [v1, v2, v3] = GENERATE(table<t1, t2, t3> data)
#define DATA4(t1, v1, t2, v2, t3, v3, t4, v4, data) \
    auto [v1, v2, v3, v4] = GENERATE(table<t1, t2, t3, t4> data)
#define DATA5(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, data) \
    auto [v1, v2, v3, v4, v5] = GENERATE(table<t1, t2, t3, t4, t5> data)
#define DATA6(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, data) \
    auto [v1, v2, v3, v4, v5, v6] = GENERATE(table<t1, t2, t3, t4, t5, t6> data)
#define DATA7(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7, data) \
    auto [v1, v2, v3, v4, v5, v6, v7] = GENERATE(table<t1, t2, t3, t4, t5, t6, t7> data)

#define DATA_PICKER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, FUNC, ...) FUNC
#define DATA(...) DATA_PICKER( \
    __VA_ARGS__, \
    DATA7, DATA7, \
    DATA6, DATA6, \
    DATA5, DATA5, \
    DATA4, DATA4, \
    DATA3, DATA3, \
    DATA2, DATA2, \
    DATA1, DATA1 \
)(__VA_ARGS__)


TEST_CASE("binutils", "[binutils]") {
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
}


TEST_CASE("CPU", "[cpu]") {
    SECTION("all instructions implemented") {
        // TODO
    }
}

TEST_CASE("Cartridge", "[cartridge]") {
    DATA(
        std::string, rom,
        std::string, title,
        ({
            { TESTS_PATH "/roms/tetris.gb", "TETRIS" },
            { TESTS_PATH "/roms/alleyway.gb", "ALLEY WAY" },
            { TESTS_PATH "/roms/pokemon-red.gb", "POKEMON RED" },
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
    return session.run();
}