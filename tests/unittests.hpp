#ifndef UNITTESTS_H
#define UNITTESTS_H

#include "testutils/catch.h"
#include "testutils/runners.hpp"
#include "utils/casts.hpp"
#include "utils/mathematics.h"
#include "utils/memory.h"

TEST_CASE("bits", "[bits]") {
    SECTION("test bit") {
        uint8_t B = 0;

        REQUIRE_FALSE(test_bit<7>(B));
        REQUIRE_FALSE(test_bit<6>(B));
        REQUIRE_FALSE(test_bit<5>(B));
        REQUIRE_FALSE(test_bit<4>(B));
        REQUIRE_FALSE(test_bit<3>(B));
        REQUIRE_FALSE(test_bit<2>(B));
        REQUIRE_FALSE(test_bit<1>(B));
        REQUIRE_FALSE(test_bit<0>(B));

        B = ~B;

        REQUIRE(test_bit<7>(B));
        REQUIRE(test_bit<6>(B));
        REQUIRE(test_bit<5>(B));
        REQUIRE(test_bit<4>(B));
        REQUIRE(test_bit<3>(B));
        REQUIRE(test_bit<2>(B));
        REQUIRE(test_bit<1>(B));
        REQUIRE(test_bit<0>(B));

        B -= 1;

        REQUIRE(B == 0xFE);
        REQUIRE_FALSE(test_bit<0>(B));
    }

    SECTION("get bit") {
        uint8_t B = 0b10101110;
        REQUIRE(get_bit<7>(B) == 0b10000000);
        REQUIRE(get_bit<6>(B) == 0b0000000);
        REQUIRE(get_bit<5>(B) == 0b100000);
        REQUIRE(get_bit<4>(B) == 0b00000);
        REQUIRE(get_bit<3>(B) == 0b1000);
        REQUIRE(get_bit<2>(B) == 0b100);
        REQUIRE(get_bit<1>(B) == 0b10);
        REQUIRE(get_bit<0>(B) == 0);
    }

    SECTION("set bit") {
        uint8_t B = 0b0000;
        set_bit<2>(B);
        REQUIRE(B == 0b0100);

        set_bit<1>(B);
        REQUIRE(B == 0b0110);
    }

    SECTION("toggle bit") {
        uint8_t B = 0b0000;
        toggle_bit<0>(B);
        REQUIRE(B == 0b0001);

        toggle_bit<0>(B);
        REQUIRE(B == 0b0000);

        toggle_bit<2>(B);
        REQUIRE(B == 0b0100);

        toggle_bit<1>(B);
        REQUIRE(B == 0b0110);

        toggle_bit<0>(B);
        REQUIRE(B == 0b0111);

        toggle_bit<2>(B);
        REQUIRE(B == 0b0011);
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
        REQUIRE(concat(A, B) == 0x0301);
        REQUIRE(concat(B, A) == 0x0103);
    }

    SECTION("bitmask_on") {
        REQUIRE(bitmask<0> == 0b00000000);
        REQUIRE(bitmask<1> == 0b00000001);
        REQUIRE(bitmask<2> == 0b00000011);
        REQUIRE(bitmask<3> == 0b00000111);
        REQUIRE(bitmask<4> == 0b00001111);
        REQUIRE(bitmask<5> == 0b00011111);
        REQUIRE(bitmask<6> == 0b00111111);
        REQUIRE(bitmask<7> == 0b01111111);
        REQUIRE(bitmask<8> == 0b11111111);
    }

    SECTION("bitmask_off") {
        REQUIRE((uint8_t)bitmask_off<0> == 0b11111111);
        REQUIRE((uint8_t)bitmask_off<1> == 0b11111110);
        REQUIRE((uint8_t)bitmask_off<2> == 0b11111100);
        REQUIRE((uint8_t)bitmask_off<3> == 0b11111000);
        REQUIRE((uint8_t)bitmask_off<4> == 0b11110000);
        REQUIRE((uint8_t)bitmask_off<5> == 0b11100000);
        REQUIRE((uint8_t)bitmask_off<6> == 0b11000000);
        REQUIRE((uint8_t)bitmask_off<7> == 0b10000000);
        REQUIRE((uint8_t)bitmask_off<8> == 0b00000000);
    }

    SECTION("bitmask_range") {
        REQUIRE(bitmask_range<0, 0> == 0b00000001);
        REQUIRE(bitmask_range<1, 0> == 0b00000011);
        REQUIRE(bitmask_range<2, 0> == 0b00000111);
        REQUIRE(bitmask_range<3, 0> == 0b00001111);
        REQUIRE(bitmask_range<4, 0> == 0b00011111);
        REQUIRE(bitmask_range<5, 0> == 0b00111111);
        REQUIRE(bitmask_range<6, 0> == 0b01111111);
        REQUIRE(bitmask_range<7, 0> == 0b11111111);

        REQUIRE(bitmask_range<0, 0> == 0b00000001);
        REQUIRE(bitmask_range<1, 0> == 0b00000011);
        REQUIRE(bitmask_range<2, 0> == 0b00000111);
        REQUIRE(bitmask_range<3, 0> == 0b00001111);
        REQUIRE(bitmask_range<4, 0> == 0b00011111);
        REQUIRE(bitmask_range<5, 0> == 0b00111111);
        REQUIRE(bitmask_range<6, 0> == 0b01111111);
        REQUIRE(bitmask_range<7, 0> == 0b11111111);

        REQUIRE(bitmask_range<2, 2> == 0b00000100);
        REQUIRE(bitmask_range<3, 2> == 0b00001100);
        REQUIRE(bitmask_range<4, 2> == 0b00011100);
        REQUIRE(bitmask_range<5, 2> == 0b00111100);
        REQUIRE(bitmask_range<6, 2> == 0b01111100);
        REQUIRE(bitmask_range<7, 2> == 0b11111100);
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
    }

    SECTION("sum_carry") {
        sum_carry_result<uint8_t> result {};

        result = sum_carry<0, uint8_t, uint8_t>(0b010, 0b101);
        REQUIRE(result.sum == 0b010 + 0b101);
        REQUIRE_FALSE(result.carry);

        result = sum_carry<0, uint8_t, uint8_t>(0b011, 0b101);
        REQUIRE(result.sum == 0b011 + 0b101);
        REQUIRE(result.carry);

        result = sum_carry<7, uint8_t, uint8_t>(0b10010101, 0b10100101);
        REQUIRE(result.sum == (uint8_t)(0b10010101 + 0b10100101));
        REQUIRE(result.carry);

        result = sum_carry<7, uint8_t, uint8_t>(0b10010101, 0b00010101);
        REQUIRE(result.sum == (uint8_t)(0b10010101 + 0b00010101));
        REQUIRE_FALSE(result.carry);
    }

    SECTION("inc_carry") {
        sum_carry_result<int> result {};

        result = inc_carry<0>(0b010);
        REQUIRE(result.sum == 0b010 + 1);
        REQUIRE_FALSE(result.carry);

        result = inc_carry<0>(0b011);
        REQUIRE(result.sum == 0b011 + 1);
        REQUIRE(result.carry);
    }

    SECTION("sum_test_carry_bit") {
        REQUIRE_FALSE(sum_test_carry_bit<0>(0b010, 0b111));
        REQUIRE(sum_test_carry_bit<1>(0b010, 0b111));
        REQUIRE(sum_test_carry_bit<2>(0b010, 0b111));
        REQUIRE_FALSE(sum_test_carry_bit<3>(0b010, 0b111));
    }

    SECTION("inc_test_carry_bit") {
        REQUIRE_FALSE(inc_test_carry_bit<0>(0b010));
        REQUIRE_FALSE(inc_test_carry_bit<1>(0b010));
        REQUIRE_FALSE(inc_test_carry_bit<2>(0b010));

        REQUIRE(inc_test_carry_bit<0>(0b011));
        REQUIRE_FALSE(inc_test_carry_bit<1>(0b010));
        REQUIRE_FALSE(inc_test_carry_bit<2>(0b010));
    }
}

TEST_CASE("math", "[math]") {
    SECTION("mod") {
        REQUIRE(mod<4>(0) == 0);
        REQUIRE(mod<4>(1) == 1);
        REQUIRE(mod<4>(2) == 2);
        REQUIRE(mod<4>(3) == 3);
        REQUIRE(mod<4>(4) == 0);
        REQUIRE(mod<4>(5) == 1);
        REQUIRE(mod<4>(6) == 2);
        REQUIRE(mod<4>(7) == 3);

        REQUIRE(mod<5>(0) == 0);
        REQUIRE(mod<5>(1) == 1);
        REQUIRE(mod<5>(2) == 2);
        REQUIRE(mod<5>(3) == 3);
        REQUIRE(mod<5>(4) == 4);
        REQUIRE(mod<5>(5) == 0);
        REQUIRE(mod<5>(6) == 1);
        REQUIRE(mod<5>(7) == 2);
    }

    SECTION("log") {
        REQUIRE(log_2<1> == 0);
        REQUIRE(log_2<2> == 1);
        REQUIRE(log_2<4> == 2);
        REQUIRE(log_2<8> == 3);
    }

    SECTION("is_power_of_2") {
        REQUIRE(is_power_of_2<1>);
        REQUIRE(is_power_of_2<2>);
        REQUIRE(is_power_of_2<4>);
        REQUIRE(is_power_of_2<8>);

        REQUIRE_FALSE(is_power_of_2<3>);
        REQUIRE_FALSE(is_power_of_2<5>);
        REQUIRE_FALSE(is_power_of_2<6>);
        REQUIRE_FALSE(is_power_of_2<9>);
    }
}

TEST_CASE("casts", "[casts]") {
    SECTION("unsigned_to_signed") {
        REQUIRE(to_signed((uint8_t)0) == 0);
        REQUIRE(to_signed((uint8_t)127) == 127);
        REQUIRE(to_signed((uint8_t)128) == -128);
        REQUIRE(to_signed((uint8_t)129) == -127);
        REQUIRE(to_signed((uint8_t)255) == -1);
    }
}

TEST_CASE("memory", "[memory]") {
    SECTION("mem_find_first") {
        {
            uint8_t haystack[] {1, 4, 6, 2, 7, 3, 6, 9, 4, 2, 3};
            uint8_t needle[] {1};
            REQUIRE(mem_find_first(haystack, array_size(haystack), needle, array_size(needle)) == 0);
        }
        {
            uint8_t haystack[] {1, 4, 6, 2, 7, 3, 6, 9, 4, 2, 3};
            uint8_t needle[] {2, 7, 3};
            REQUIRE(mem_find_first(haystack, array_size(haystack), needle, array_size(needle)) == 3);
        }
        {
            uint8_t haystack[] {1, 4, 6, 2, 2, 2, 7, 3, 4, 2, 3};
            uint8_t needle[] {2, 7, 3};
            REQUIRE(mem_find_first(haystack, array_size(haystack), needle, array_size(needle)) == 5);
        }
        {
            uint8_t haystack[] {1, 4, 6, 2, 2, 2, 7, 3, 4, 2, 3};
            uint8_t needle[] {2, 7, 4};
            REQUIRE(mem_find_first(haystack, array_size(haystack), needle, array_size(needle)) == std::nullopt);
        }
        {
            uint8_t haystack[] {1, 4, 6, 2, 2, 2, 7, 3, 4, 2, 3};
            uint8_t needle[] {3, 4, 2, 3, 5};
            REQUIRE(mem_find_first(haystack, array_size(haystack), needle, array_size(needle)) == std::nullopt);
        }
    }
    SECTION("mem_find_all") {
        {
            uint8_t haystack[] {1, 4, 6, 2, 1, 3, 6, 1, 4, 2, 3};
            uint8_t needle[] {1};
            REQUIRE(mem_find_all(haystack, array_size(haystack), needle, array_size(needle)) ==
                    std::vector<uint32_t> {0, 4, 7});
        }
        {
            uint8_t haystack[] {1, 4, 6, 2, 1, 3, 6, 1, 4, 2, 3};
            uint8_t needle[] {0};
            REQUIRE(mem_find_all(haystack, array_size(haystack), needle, array_size(needle)).empty());
        }
        {
            uint8_t haystack[] {1, 4, 6, 2, 1, 3, 6, 1, 4, 2, 3};
            uint8_t needle[] {1, 4};
            REQUIRE(mem_find_all(haystack, array_size(haystack), needle, array_size(needle)) ==
                    std::vector<uint32_t> {0, 7});
        }
        {
            uint8_t haystack[] {1, 4, 6, 2, 1, 3, 6, 1, 4, 2, 1};
            uint8_t needle[] {1, 4};
            REQUIRE(mem_find_all(haystack, array_size(haystack), needle, array_size(needle)) ==
                    std::vector<uint32_t> {0, 7});
        }
        {
            uint8_t haystack[] {1, 4, 6, 2, 1, 3, 6, 1, 4, 2, 1};
            uint8_t needle[] {4, 6, 2};
            REQUIRE(mem_find_all(haystack, array_size(haystack), needle, array_size(needle)) ==
                    std::vector<uint32_t> {1});
        }
        {
            uint8_t haystack[] {1, 4, 4, 6, 2, 1, 6, 1, 4, 2, 1};
            uint8_t needle[] {4, 6, 2};
            REQUIRE(mem_find_all(haystack, array_size(haystack), needle, array_size(needle)) ==
                    std::vector<uint32_t> {2});
        }
        {
            uint8_t haystack[] {1, 2, 1, 2, 2, 1, 2, 1, 4, 2, 1};
            uint8_t needle[] {2, 1, 2};
            REQUIRE(mem_find_all(haystack, array_size(haystack), needle, array_size(needle)) ==
                    std::vector<uint32_t> {1, 4});
        }
        {
            uint8_t haystack[] {1, 2, 1, 2, 2, 1, 2, 1, 4, 2, 1};
            uint8_t needle[] {4, 2, 1, 8};
            REQUIRE(mem_find_all(haystack, array_size(haystack), needle, array_size(needle)).empty());
        }
        {
            uint8_t haystack[] {1, 1, 1, 1};
            uint8_t needle[] {1, 1, 1};
            REQUIRE(mem_find_all(haystack, array_size(haystack), needle, array_size(needle)) ==
                    std::vector<uint32_t> {0, 1});
        }
    }
}

TEST_CASE("adt", "[adt]") {
    SECTION("Vector") {
        Vector<uint8_t, 8> v;
        REQUIRE(v.isEmpty());

        v.pushBack(1);
        v.pushBack(2);
        v.pushBack(3);
        v.pushBack(4);

        REQUIRE(v.size() == 4);

        REQUIRE(v.popBack() == 4);
        REQUIRE(v.popBack() == 3);
        REQUIRE(v.popBack() == 2);
        REQUIRE(v.popBack() == 1);

        REQUIRE(v.isEmpty());

        v.pushBack(1);
        v.pushBack(2);
        v.pushBack(3);
        v.pushBack(4);
        v.pushBack(5);
        v.pushBack(6);
        v.pushBack(7);
        v.pushBack(8);

        REQUIRE(v.isFull());
    }

    SECTION("Queue") {
        Queue<uint8_t, 8> q;
        REQUIRE(q.isEmpty());

        q.pushBack(1);
        q.pushBack(2);
        REQUIRE(q.isNotEmpty());
        REQUIRE(q.size() == 2);

        q.clear();
        REQUIRE(q.isEmpty());

        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        q.pushBack(4);
        q.pushBack(5);
        q.pushBack(6);
        q.pushBack(7);
        q.pushBack(8);

        REQUIRE(q[7] == 8);

        REQUIRE(q.isFull());

        REQUIRE(q.popFront() == 1);
        REQUIRE(q.popFront() == 2);
        REQUIRE(q.popFront() == 3);
        REQUIRE(q.popFront() == 4);
        REQUIRE(q.popFront() == 5);
        REQUIRE(q.popFront() == 6);
        REQUIRE(q.popFront() == 7);
        REQUIRE(q.popFront() == 8);

        REQUIRE(q.isEmpty());

        q.pushBack(1);
        q.pushBack(2);

        REQUIRE(q.size() == 2);

        q.clear();
        REQUIRE(q.isEmpty());

        q.pushBack(3);
        q.pushBack(4);

        REQUIRE(q[0] == 3);
        REQUIRE(q[1] == 4);

        REQUIRE(q.size() == 2);
        REQUIRE(q.popFront() == 3);
        REQUIRE(q.size() == 1);
        REQUIRE(q.popFront() == 4);
        REQUIRE(q.isEmpty());

        q.pushBack(3);
        q.pushBack(4);

        REQUIRE(q[0] == 3);
        REQUIRE(q[1] == 4);

        REQUIRE(q.popFront() == 3);

        REQUIRE(q[0] == 4);
    }

    SECTION("FillQueue") {
        FillQueue<uint8_t, 8> q;
        REQUIRE(q.isEmpty());

        uint64_t x = 1;
        q.fill(&x);

        REQUIRE(q.isFull());

        REQUIRE(q.popFront() == 1);
        REQUIRE(q.popFront() == 0);
        REQUIRE(q.popFront() == 0);
        REQUIRE(q.popFront() == 0);

        REQUIRE(q.size() == 4);

        REQUIRE(q.popFront() == 0);
        REQUIRE(q.popFront() == 0);
        REQUIRE(q.popFront() == 0);
        REQUIRE(q.popFront() == 0);

        REQUIRE(q.isEmpty());
    }
}

TEST_CASE("parcel", "[parcel]") {
    SECTION("Parcel") {
        Parcel p;

        bool b = true;
        uint8_t u8 = 10;
        uint16_t u16 = 231;
        uint32_t u32 = 15523432;
        uint64_t u64 = 155234124142432;

        int8_t i8 = 10;
        int16_t i16 = 231;
        int32_t i32 = 15523432;
        int64_t i64 = 155234124142432;

        uint8_t B[4] {0x10, 0x42, 0xFF, 0xA4};

        p.writeBool(b);
        p.writeUInt8(u8);
        p.writeUInt16(u16);
        p.writeUInt32(u32);
        p.writeUInt64(u64);
        p.writeInt8(i8);
        p.writeInt16(i16);
        p.writeInt32(i32);
        p.writeInt64(i64);
        p.writeBytes(B, 4);

        REQUIRE(p.readBool() == b);
        REQUIRE(p.readUInt8() == u8);
        REQUIRE(p.readUInt16() == u16);
        REQUIRE(p.readUInt32() == u32);
        REQUIRE(p.readUInt64() == u64);
        REQUIRE(p.readInt8() == i8);
        REQUIRE(p.readInt16() == i16);
        REQUIRE(p.readInt32() == i32);
        REQUIRE(p.readInt64() == i64);

        uint8_t BB[4];
        p.readBytes(BB, 4);
        REQUIRE(memcmp(BB, B, 4) == 0);

        REQUIRE(p.getRemainingSize() == 0);
    }
}

TEST_CASE("state", "[state]") {
    SECTION("Save/Load: Round 1") {
        std::vector<uint8_t> data1;

        {
            SimpleRunner runner;
            runner.rom(TESTS_ROOT_FOLDER "/roms/blargg/cpu_instrs.gb").maxTicks(10'000).run();
            data1.resize(runner.core.getStateSize());
            runner.core.saveState(data1.data());
        }

        std::vector<uint8_t> data2(data1.size());

        {
            SimpleRunner runner;
            runner.rom(TESTS_ROOT_FOLDER "/roms/blargg/cpu_instrs.gb");
            runner.core.loadState(data1.data());
            runner.core.saveState(data2.data());
        }

        REQUIRE(memcmp(data1.data(), data2.data(), data1.size()) == 0);
    }
    SECTION("Save/Load: Round 2") {
        std::vector<uint8_t> data1;
        std::vector<uint8_t> data2;
        std::vector<uint8_t> data3;

        {
            SimpleRunner runner;
            runner.rom(TESTS_ROOT_FOLDER "/roms/blargg/cpu_instrs.gb");

            runner.maxTicks(1'000'000).run();
            data1.resize(runner.core.getStateSize());
            data2.resize(runner.core.getStateSize());
            data3.resize(runner.core.getStateSize());
            runner.core.saveState(data1.data());

            runner.maxTicks(2'000'000).run();
            runner.core.saveState(data2.data());
        }

        {
            SimpleRunner runner;
            runner.rom(TESTS_ROOT_FOLDER "/roms/blargg/cpu_instrs.gb");

            runner.core.loadState(data1.data());
            runner.maxTicks(2'000'000).run();
            runner.core.saveState(data3.data());
        }

        REQUIRE(memcmp(data2.data(), data3.data(), data2.size()) == 0);
    }
}

#endif // UNITTESTS_H