#define CATCH_CONFIG_RUNNER

#include "catch2/catch_all.hpp"
#include "docboy/core/core.h"
#include "docboy/gameboy/gameboy.h"
#include "extra/serial/endpoints/buffer.h"
#include "testutils/img.h"
#include "utils/bits.hpp"
#include "utils/casts.hpp"
#include "utils/fillqueue.hpp"
#include "utils/formatters.hpp"
#include "utils/hexdump.hpp"
#include "utils/memory.h"
#include "utils/parcel.h"
#include "utils/path.h"
#include "utils/queue.hpp"
#include "utils/vector.hpp"
#include <SDL.h>
#include <SDL_image.h>
#include <optional>
#include <utility>
#include <variant>

#include "os.h"

#define ENABLE_DOCBOY_TEST_ROMS 1

#define TABLE_1(t1, data) GENERATE(table<t1> data)
#define TABLE_2(t1, t2, data) GENERATE(table<t1, t2> data)
#define TABLE_3(t1, t2, t3, data) GENERATE(table<t1, t2, t3> data)
#define TABLE_4(t1, t2, t3, t4, data) GENERATE(table<t1, t2, t3, t4> data)

#define TABLE_PICKER(_1, _2, _3, _4, _5, FUNC, ...) FUNC
#define TABLE(...) TABLE_PICKER(__VA_ARGS__, TABLE_4, TABLE_3, TABLE_2, TABLE_1)(__VA_ARGS__)

static const std::string TEST_ROMS_PATH = "tests/roms/";
static const std::string TEST_RESULTS_PATH = "tests/results/";

using Palette = std::vector<uint16_t>;

static const Palette DEFAULT_PALETTE {};
static const Palette GREY_PALETTE {0xFFFF, 0xAD55, 0x52AA, 0x0000}; // {0xFF, 0xAA, 0x55, 0x00} in RGB565

static constexpr uint64_t DURATION_VERY_LONG = 250'000'000;
static constexpr uint64_t DURATION_LONG = 100'000'000;
static constexpr uint64_t DURATION_MEDIUM = 30'000'000;
static constexpr uint64_t DURATION_SHORT = 5'000'000;
static constexpr uint64_t DURATION_VERY_SHORT = 1'500'000;

static constexpr uint64_t DEFAULT_DURATION = DURATION_LONG;

template <typename RunnerImpl>
class RunnerT {
public:
    RunnerImpl& rom(const std::string& filename) {
        romName = filename;
        core.loadRom(filename);
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& maxTicks(uint64_t ticks) {
        maxTicks_ = ticks;
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& checkIntervalTicks(uint64_t ticks) {
        checkIntervalTicks_ = ticks / 4 * 4; // should be a multiple of 4
        return static_cast<RunnerImpl&>(*this);
    }

    RunnerImpl& stopAtInstruction(std::optional<uint8_t> instr) {
        stopAtInstruction_ = instr;
        return static_cast<RunnerImpl&>(*this);
    }

    bool run() {
#if !ENABLE_DOCBOY_TEST_ROMS
        // Just skip the rom
        if (romName.find("docboy") != std::string::npos)
            return true;
#endif

        auto* impl = static_cast<RunnerImpl*>(this);
        impl->onRun();

        bool hasEverChecked {false};

        for (tick = core.ticks; tick <= maxTicks_ && canRun; tick += 4) {
            core.cycle();
            if (impl->shouldCheckExpectation()) {
                hasEverChecked = true;
                if (impl->checkExpectation()) {
                    return true;
                }
            }
        }

        if (hasEverChecked) {
            impl->onExpectationFailed();
        } else if (impl->shouldCheckExpectation()) {
            UNSCOPED_INFO("Expectation never checked!");
        }

        return false;
    }

    std::unique_ptr<GameBoy> gb {std::make_unique<GameBoy>()};
    Core core {*gb};

protected:
    std::string romName; // debug
    uint64_t tick {};
    uint64_t maxTicks_ {UINT64_MAX};
    uint64_t checkIntervalTicks_ {UINT64_MAX};
    std::optional<uint8_t> stopAtInstruction_ {};

    bool canRun {true};
};

class Runner : public RunnerT<Runner> {
public:
    void onRun() {
    }
    bool shouldCheckExpectation() {
        return false;
    }
    bool checkExpectation() {
        return true;
    }
    void onExpectationFailed() {
    }
};

class FramebufferRunner : public RunnerT<FramebufferRunner> {
public:
    FramebufferRunner& expectFramebuffer(const std::string& filename, const Palette& colors) {
        load_png(filename, expectedFramebuffer, SDL_PIXELFORMAT_RGB565);
        pixelColors = colors;
        return *this;
    }

    void onRun() {
    }

    bool shouldCheckExpectation() {
        // Schedule a check for the next VBlank if we reached the required instruction
        if (stopAtInstruction_) {
            if (core.gb.cpu.instruction.opcode == *stopAtInstruction_) {
                // Force check now
                canRun = false;
                return true;
            }
        }

        // Schedule a check for the next VBlank if we have passed the check interval
        if (tick > 0 && tick % checkIntervalTicks_ == 0) {
            pendingCheck = true;
        }

        // Check if we are in VBlank with a pending check
        if (pendingCheck && keep_bits<2>(core.gb.video.STAT) == 1) {
            pendingCheck = false;
            return true;
        }

        return false;
    }

    bool checkExpectation() {
        memcpy(lastFramebuffer, gb->lcd.getPixels(), FRAMEBUFFER_SIZE);
        if (!pixelColors.empty())
            convert_framebuffer_pixels(gb->lcd.getPixels(), Lcd::RGB565_PALETTE, lastFramebuffer, pixelColors.data());
        return are_framebuffer_equals(lastFramebuffer, expectedFramebuffer);
    }

    void onExpectationFailed() {
        // Framebuffer not equals: figure out where's the (first) problem
        UNSCOPED_INFO("=== " << romName << " ===");

        uint32_t i;
        for (i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
            if (lastFramebuffer[i] != expectedFramebuffer[i]) {
                UNSCOPED_INFO("Framebuffer mismatch at position "
                              << i << " (row=" << i / 160 << ", column=" << i % 160 << ": (actual) 0x"
                              << hex(lastFramebuffer[i]) << " != (expected) 0x" << hex(expectedFramebuffer[i]));
                break;
            }
        }
        check(i < FRAMEBUFFER_NUM_PIXELS);

        // Dump framebuffers

        const auto tmpPath = temp_directory_path() / "docboy";
        create_directory(tmpPath.string());

        const auto pathActual = (tmpPath / path {path {romName}.filename() + "-actual.png"}).string();
        const auto pathExpected = (tmpPath / path {path {romName}.filename() + "-expected.png"}).string();
        save_framebuffer_as_png(pathActual, lastFramebuffer);
        save_framebuffer_as_png(pathExpected, expectedFramebuffer);
        UNSCOPED_INFO("You can find the PNGs of the framebuffers at " << pathActual << " and " << pathExpected);
    }

private:
    uint16_t lastFramebuffer[FRAMEBUFFER_NUM_PIXELS] {};
    uint16_t expectedFramebuffer[FRAMEBUFFER_NUM_PIXELS] {};
    Palette pixelColors {};
    bool pendingCheck {};
};

class SerialRunner : public RunnerT<SerialRunner> {
public:
    SerialRunner& expectOutput(const std::vector<uint8_t>& output) {
        expectedOutput = output;
        return *this;
    }

    void onRun() {
        serialLink.plug1.attach(serialBuffer);
        core.attachSerialLink(serialLink.plug2);
    }

    bool shouldCheckExpectation() {
        return tick > 0 && tick % checkIntervalTicks_ == 0;
    }

    bool checkExpectation() {
        lastOutput = serialBuffer.buffer;
        return serialBuffer.buffer == expectedOutput;
    }

    void onExpectationFailed() {
        UNSCOPED_INFO("=== " << romName << " ===");
        UNSCOPED_INFO("Expected serial output: " << hex(expectedOutput));
        UNSCOPED_INFO("Actual serial output  : " << hex(lastOutput));
    }

private:
    SerialBuffer serialBuffer;
    SerialLink serialLink;
    std::vector<uint8_t> lastOutput;
    std::vector<uint8_t> expectedOutput;
};

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
    SECTION("State Round 1") {
        std::vector<uint8_t> data1;

        {
            Runner runner;
            runner.rom("tests/roms/blargg/cpu_instrs.gb").maxTicks(10'000).run();
            data1.resize(runner.core.getStateSaveSize());
            runner.core.saveState(data1.data());
        }

        std::vector<uint8_t> data2(data1.size());

        {
            Runner runner;
            runner.rom("tests/roms/blargg/cpu_instrs.gb");
            runner.core.loadState(data1.data());
            runner.core.saveState(data2.data());
        }

        REQUIRE(memcmp(data1.data(), data2.data(), data1.size()) == 0);
    }
    SECTION("State Round 2") {
        std::vector<uint8_t> data1;
        std::vector<uint8_t> data2;
        std::vector<uint8_t> data3;

        {
            Runner runner;
            runner.rom("tests/roms/blargg/cpu_instrs.gb");

            runner.maxTicks(1'000'000).run();
            data1.resize(runner.core.getStateSaveSize());
            data2.resize(runner.core.getStateSaveSize());
            data3.resize(runner.core.getStateSaveSize());
            runner.core.saveState(data1.data());

            runner.maxTicks(2'000'000).run();
            runner.core.saveState(data2.data());
        }

        {
            Runner runner;
            runner.rom("tests/roms/blargg/cpu_instrs.gb");

            runner.core.loadState(data1.data());
            runner.maxTicks(2'000'000).run();
            runner.core.saveState(data3.data());
        }

        REQUIRE(memcmp(data2.data(), data3.data(), data2.size()) == 0);
    }
}

struct MaxTicks {
    uint64_t value;
};
struct StopAtInstruction {
    uint8_t instruction;
};
using RunnerParam = std::variant<std::monostate, Palette, MaxTicks, StopAtInstruction>;

struct FramebufferRunnerParams {

    FramebufferRunnerParams(std::string&& rom, std::string&& expected, RunnerParam param1 = std::monostate {}) :
        rom(std::move(rom)) {
        result = std::move(expected);

        if (std::holds_alternative<Palette>(param1)) {
            palette = std::get<Palette>(param1);
        } else if (std::holds_alternative<MaxTicks>(param1)) {
            maxTicks = std::get<MaxTicks>(param1).value;
        } else if (std::holds_alternative<StopAtInstruction>(param1)) {
            stopAtInstruction = std::get<StopAtInstruction>(param1).instruction;
        }
    }

    std::string rom;
    std::string result;
    Palette palette {DEFAULT_PALETTE};
    uint64_t maxTicks {DEFAULT_DURATION};
    std::optional<uint8_t> stopAtInstruction {};
};

struct SerialRunnerParams {
    SerialRunnerParams(std::string&& rom, std::vector<uint8_t>&& expected, uint64_t maxTicks_ = DURATION_MEDIUM) :
        rom(std::move(rom)) {
        result = std::move(expected);
        maxTicks = maxTicks_;
    }

    std::string rom;
    std::vector<uint8_t> result;
    uint64_t maxTicks;
};

using F = FramebufferRunnerParams;
using S = SerialRunnerParams;
using RunnerParams = std::variant<F, S>;

static bool run_with_params(const RunnerParams& p) {
    if (std::holds_alternative<FramebufferRunnerParams>(p)) {
        const auto pf = std::get<FramebufferRunnerParams>(p);
        INFO("=== " << TEST_ROMS_PATH + pf.rom << " ===");
        return FramebufferRunner()
            .rom(TEST_ROMS_PATH + pf.rom)
            .maxTicks(pf.maxTicks)
            .checkIntervalTicks(DURATION_VERY_SHORT)
            .stopAtInstruction(pf.stopAtInstruction)
            .expectFramebuffer(TEST_RESULTS_PATH + pf.result, pf.palette)
            .run();
    }

    if (std::holds_alternative<SerialRunnerParams>(p)) {
        const auto ps = std::get<SerialRunnerParams>(p);
        INFO("=== " << TEST_ROMS_PATH + ps.rom << " ===");
        return SerialRunner()
            .rom(TEST_ROMS_PATH + ps.rom)
            .maxTicks(ps.maxTicks)
            .checkIntervalTicks(DURATION_VERY_SHORT)
            .expectOutput(ps.result)
            .run();
    }

    checkNoEntry();
    return false;
}

#define RUN_TEST_ROMS(...)                                                                                             \
    const auto [params] = TABLE(RunnerParams, ({__VA_ARGS__}));                                                        \
    REQUIRE(run_with_params(params))

#define ALL_TEST_ROMS 0
#define PPU_ONLY_TEST_ROMS 0
#define MEALYBUG_ONLY_TEST_ROMS 1
#define WIP_ONLY_TEST_ROMS 0

TEST_CASE("emulation", "[emulation][.]") {
#if ALL_TEST_ROMS || PPU_ONLY_TEST_ROMS

#if ALL_TEST_ROMS
    SECTION("cpu") {
        RUN_TEST_ROMS(F {"blargg/cpu_instrs.gb", "blargg/cpu_instrs.png", MaxTicks {DURATION_VERY_LONG}},
                      F {"blargg/instr_timing.gb", "blargg/instr_timing.png"},
                      F {"blargg/halt_bug.gb", "blargg/halt_bug.png", MaxTicks {DURATION_VERY_LONG}},
                      F {"blargg/mem_timing.gb", "blargg/mem_timing.png"},
                      F {"blargg/mem_timing-2.gb", "blargg/mem_timing-2.png"},
                      S {"mooneye/instr/daa.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/bits/reg_f.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/add_sp_e_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/call_cc_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/call_cc_timing2.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/call_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/call_timing2.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/di_timing-GS.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/ei_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/jp_cc_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/jp_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/ld_hl_sp_e_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/pop_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/push_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/ret_cc_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/ret_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/reti_intr_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/reti_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/rst_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/halt_ime0_ei.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/halt_ime0_nointr_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/halt_ime1_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/halt_ime1_timing2-GS.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},

        );
    }
#endif

    SECTION("ppu") {
        RUN_TEST_ROMS(
            // dmg-acid2
            F {"dmg-acid2/dmg-acid2.gb", "dmg-acid2/dmg-acid2.png", GREY_PALETTE},

            // mooneye
            S {"mooneye/ppu/hblank_ly_scx_timing-GS.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/intr_1_2_timing-GS.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/intr_2_0_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/intr_2_mode0_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/intr_2_mode0_timing_sprites.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/intr_2_mode3_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/intr_2_oam_ok_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/lcdon_timing-GS.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/lcdon_write_timing-GS.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/stat_irq_blocking.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/stat_lyc_onoff.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
            S {"mooneye/ppu/vblank_stat_intr-GS.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},

            // mealybug
            F {"mealybug/m2_win_en_toggle.gb", "mealybug/m2_win_en_toggle.png", GREY_PALETTE},
            F {"mealybug/m3_bgp_change.gb", "mealybug/m3_bgp_change.png", GREY_PALETTE},
            F {"mealybug/m3_bgp_change_sprites.gb", "mealybug/m3_bgp_change_sprites.png", GREY_PALETTE},
            //            F{"mealybug/m3_lcdc_bg_en_change.gb", "mealybug/m3_lcdc_bg_en_change.png",
            //                GREY_PALETTE}, F{"mealybug/m3_lcdc_bg_map_change.gb",
            //                "mealybug/m3_lcdc_bg_map_change.png", GREY_PALETTE},
            //                F{"mealybug/m3_lcdc_obj_en_change.gb", "mealybug/m3_lcdc_obj_en_change.png",
            //                GREY_PALETTE}, F{"mealybug/m3_lcdc_obj_en_change_variant.gb",
            //                "mealybug/m3_lcdc_obj_en_change_variant.png", GREY_PALETTE},
            //                F{"mealybug/m3_lcdc_obj_size_change.gb", "mealybug/m3_lcdc_obj_size_change.png",
            //                GREY_PALETTE}, F{"mealybug/m3_lcdc_obj_size_change_scx.gb",
            //                "mealybug/m3_lcdc_obj_size_change_scx.png", GREY_PALETTE},
            //                F{"mealybug/m3_lcdc_tile_sel_change.gb", "mealybug/m3_lcdc_tile_sel_change.png",
            //                GREY_PALETTE}, F{"mealybug/m3_lcdc_tile_sel_win_change.gb",
            //                "mealybug/m3_lcdc_tile_sel_win_change.png", GREY_PALETTE},
            //                F{"mealybug/m3_lcdc_win_en_change_multiple.gb",
            //                "mealybug/m3_lcdc_win_en_change_multiple.png", GREY_PALETTE},
            //                F{"mealybug/m3_lcdc_win_en_change_multiple_wx.gb",
            //                "mealybug/m3_lcdc_win_en_change_multiple_wx.png", GREY_PALETTE},
            //                F{"mealybug/m3_lcdc_win_map_change.gb", "mealybug/m3_lcdc_win_map_change.png",
            //                GREY_PALETTE}, F{"mealybug/m3_obp0_change.gb", "mealybug/m3_obp0_change.png",
            //                GREY_PALETTE},
            F {"mealybug/m3_scx_high_5_bits.gb", "mealybug/m3_scx_high_5_bits.png", GREY_PALETTE},
            F {"mealybug/m3_scx_low_3_bits.gb", "mealybug/m3_scx_low_3_bits.png", GREY_PALETTE},
            // F{"mealybug/m3_scy_change.gb", "mealybug/m3_scy_change.png", GREY_PALETTE},
            F {"mealybug/m3_window_timing.gb", "mealybug/m3_window_timing.png", GREY_PALETTE},
            F {"mealybug/m3_window_timing_wx_0.gb", "mealybug/m3_window_timing_wx_0.png", GREY_PALETTE},
            F {"mealybug/m3_wx_4_change.gb", "mealybug/m3_wx_4_change.png", GREY_PALETTE},
            F {"mealybug/m3_wx_4_change_sprites.gb", "mealybug/m3_wx_4_change_sprites.png", GREY_PALETTE},
            F {"mealybug/m3_wx_5_change.gb", "mealybug/m3_wx_5_change.png", GREY_PALETTE},
            F {"mealybug/m3_wx_6_change.gb", "mealybug/m3_wx_6_change.png", GREY_PALETTE},

            // docboy
            F {"docboy/ppu/hblank_raises_hblank_stat_interrupt.gb", "docboy/ok.png"},
            F {"docboy/ppu/hblank_raises_oam_stat_interrupt.gb", "docboy/ok.png"},
            F {"docboy/ppu/hblank_raises_vblank_stat_interrupt.gb", "docboy/ok.png"},
            F {"docboy/ppu/ly_154.gb", "docboy/ok.png"},
            F {"docboy/ppu/lyc_stat_interrupt_flag_timing_from_boot_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/lyc_stat_interrupt_flag_timing_from_boot_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/lyc_stat_interrupt_flag_timing_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/lyc_stat_interrupt_flag_timing_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/ly_timing_scx0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/ly_timing_scx0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/ly_timing_scx5_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/ly_timing_scx5_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/manual_stat_interrupt_hblank_interrupt_flag.gb", "docboy/ok.png"},
            F {"docboy/ppu/manual_stat_interrupt_hblank_interrupt.gb", "docboy/ok.png"},
            F {"docboy/ppu/manual_stat_interrupt_oam_interrupt_flag.gb", "docboy/ok.png"},
            F {"docboy/ppu/manual_stat_interrupt_oam_interrupt.gb", "docboy/ok.png"},
            F {"docboy/ppu/manual_stat_interrupt_vblank_interrupt_flag.gb", "docboy/ok.png"},
            F {"docboy/ppu/manual_stat_interrupt_vblank_interrupt_flag_reset.gb", "docboy/ok.png"},
            F {"docboy/ppu/manual_stat_interrupt_vblank_interrupt.gb", "docboy/ok.png"},
            F {"docboy/ppu/oam_irq_clears_stat_irq_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/oam_irq_clears_stat_irq_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/oam_stat_interrupt_flag_timing_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/oam_stat_interrupt_flag_timing_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/oam_stat_interrupt_ly_timing_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/oam_stat_interrupt_ly_timing_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/oam_stat_interrupt_stat_timing_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/oam_stat_interrupt_stat_timing_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/oam_stat_interrupt_stat_timing_round3.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx2_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx2_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx3_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx3_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx4_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx4_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx5_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx5_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx6_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx6_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx7_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_0_sprites_scx7_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx2_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx2_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx3_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx3_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx4_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx4_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx5_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x0_scx5_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x32_scx0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_1_sprite_x32_scx0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_2_sprites_x0_scx0_obj_disabled.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_2_sprites_x0_scx0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_2_sprites_x0_scx0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_from_boot_scx0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_from_boot_scx0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_from_boot_scx1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_from_boot_scx1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_from_boot_scx2_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_from_boot_scx2_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_from_boot_scx3_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_from_boot_scx3_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_from_boot_scx4_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_from_boot_scx4_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_0_sprites_scx0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_0_sprites_scx0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_0_sprites_scx1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_0_sprites_scx1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_0_sprites_scx2_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_0_sprites_scx2_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_0_sprites_scx3_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_0_sprites_scx3_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_0_sprites_scx4_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_0_sprites_scx4_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx2_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx2_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx3_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx3_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx4_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx4_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx5_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_stat_interrupt_1_sprite_x0_scx5_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx15_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx15_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx17_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx17_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx7_scx0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx7_scx0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx7_scx7_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx7_scx7_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx8_scx7_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx8_scx7_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx128_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx128_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx129_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx129_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx130_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx130_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx131_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx131_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx132_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx132_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx133_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx133_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx134_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx134_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx135_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_window_wx_reenable_retrigger_wx135_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_winon_bgoff_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/pixel_transfer_timing_winon_bgoff_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_off_ly2_lyc0_stat.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_off_ly2_lyc1_stat.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_off_ly2_lyc2_stat.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_off_mode_0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly0_round0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly1_round3.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly1_round4.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly1_round5.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly1_round6.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly2_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_oam_ly2_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_vram_ly0_round0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_vram_ly0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_vram_ly0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_vram_ly0_round3.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_vram_ly1_round0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_vram_ly1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_read_vram_ly1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly0_lyc0_interrupt_flag.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly0_lyc0_interrupt.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly0_lyc0_round0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly0_lyc0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly0_lyc0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly0_lyc1_round0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly0_lyc1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly0_lyc1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly1_lyc0_round0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly1_lyc0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly1_lyc0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly1_lyc1_round0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly1_lyc1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_ly1_lyc1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_pixel_transfer_ly0_round0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_pixel_transfer_ly0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_pixel_transfer_ly0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_pixel_transfer_ly1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_stat_pixel_transfer_ly1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly0_round0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly0_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly0_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly1_round0.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly1_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly1_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly1_round3.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly1_round4.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly1_round5.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly1_round6.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly1_round7.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly2_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly2_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly2_round3.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly2_round4.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly2_round5.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly2_round6.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly2_round7.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly2_sprites_round3.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly2_sprites_round4.gb", "docboy/ok.png"},
            F {"docboy/ppu/turn_on_write_oam_ly2_sprites_round5.gb", "docboy/ok.png"},
            F {"docboy/ppu/vblank_mode_timing_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/vblank_mode_timing_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/vblank_mode_timing_turn_on_round1.gb", "docboy/ok.png"},
            F {"docboy/ppu/vblank_mode_timing_turn_on_round2.gb", "docboy/ok.png"},
            F {"docboy/ppu/vblank_raises_hblank_stat_interrupt.gb", "docboy/ok.png"},
            F {"docboy/ppu/vblank_raises_oam_stat_interrupt.gb", "docboy/ok.png"},
            F {"docboy/ppu/vblank_raises_vblank_stat_interrupt.gb", "docboy/ok.png"},
            F {"docboy/ppu/write_lyc_matching_ly.gb", "docboy/ok.png"},
            F {"docboy/ppu/write_lyc_stat_change.gb", "docboy/ok.png"},
            F {"docboy/ppu/write_ly_ignored.gb", "docboy/ok.png"},
            F {"docboy/ppu/write_oam_oam_scan.gb", "docboy/ok.png"},
            F {"docboy/ppu/write_oam_pixel_transfer.gb", "docboy/ok.png"},
            F {"docboy/ppu/write_read_stat.gb", "docboy/ok.png"},
            F {"docboy/ppu/write_vram_oam_scan.gb", "docboy/ok.png"},
            F {"docboy/ppu/write_vram_pixel_transfer.gb", "docboy/ok.png"},

            F {"docboy/ppu/rendering/window_bg_reprise_wx10.gb", "docboy/ppu/window_bg_reprise_wx10.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx11.gb", "docboy/ppu/window_bg_reprise_wx11.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx12.gb", "docboy/ppu/window_bg_reprise_wx12.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx13.gb", "docboy/ppu/window_bg_reprise_wx13.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx14.gb", "docboy/ppu/window_bg_reprise_wx14.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx15.gb", "docboy/ppu/window_bg_reprise_wx15.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx16.gb", "docboy/ppu/window_bg_reprise_wx16.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx17.gb", "docboy/ppu/window_bg_reprise_wx17.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx18.gb", "docboy/ppu/window_bg_reprise_wx18.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx19.gb", "docboy/ppu/window_bg_reprise_wx19.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx20.gb", "docboy/ppu/window_bg_reprise_wx20.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx21.gb", "docboy/ppu/window_bg_reprise_wx21.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx22.gb", "docboy/ppu/window_bg_reprise_wx22.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx23.gb", "docboy/ppu/window_bg_reprise_wx23.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx24.gb", "docboy/ppu/window_bg_reprise_wx24.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx25.gb", "docboy/ppu/window_bg_reprise_wx25.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx8.gb", "docboy/ppu/window_bg_reprise_wx8.png"},
            F {"docboy/ppu/rendering/window_bg_reprise_wx9.gb", "docboy/ppu/window_bg_reprise_wx9.png"},
            F {"docboy/ppu/rendering/window_move_wy_ahead.gb", "docboy/ppu/window_move_wy_ahead.png"},
            F {"docboy/ppu/rendering/window_move_wy_behind_change_wx.gb",
               "docboy/ppu/window_move_wy_behind_change_wx.png"},
            F {"docboy/ppu/rendering/window_move_wy_behind.gb", "docboy/ppu/window_move_wy_behind.png"},
            F {"docboy/ppu/rendering/window_retrigger_increase_wly_round1.gb",
               "docboy/ppu/window_retrigger_increase_wly_round1.png"},
            F {"docboy/ppu/rendering/window_retrigger_increase_wly_round2.gb",
               "docboy/ppu/window_retrigger_increase_wly_round2.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx0_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx0_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx0_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx0_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx100.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx100.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx101.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx101.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx102.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx102.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx103.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx103.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx81.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx81.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx82.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx82.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx83.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx83.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx84.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx84.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx85.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx85.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx86.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx86.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx87.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx87.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx88.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx88.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx89.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx89.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx90.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx90.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx91.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx91.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx92.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx92.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx93.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx93.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx94.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx94.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx95.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx95.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx96.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx96.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx97.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx97.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx98.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx98.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx100_wx99.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx100_wx99.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx100.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx100.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx101.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx101.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx102.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx102.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx103.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx103.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx79_change_during_hblank.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx79_change_during_hblank.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx79_change_during_oam_scan.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx79_change_during_oam_scan.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx80_change_during_hblank.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx80_change_during_hblank.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx80_change_during_oam_scan.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx80_change_during_oam_scan.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx81.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx81.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx82.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx82.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx83.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx83.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx84.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx84.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx85.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx85.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx86.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx86.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx87.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx87.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx88.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx88.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx89.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx89.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx90.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx90.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx91.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx91.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx92.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx92.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx93.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx93.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx94.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx94.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx95.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx95.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx96.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx96.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx97.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx97.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx98.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx98.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx103_wx99.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx103_wx99.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx100.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx100.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx101.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx101.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx102.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx102.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx103.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx103.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx81.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx81.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx82.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx82.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx83.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx83.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx84.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx84.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx85.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx85.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx86.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx86.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx87.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx87.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx88.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx88.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx89.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx89.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx90.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx90.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx91.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx91.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx92.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx92.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx93.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx93.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx94.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx94.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx95.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx95.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx96.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx96.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx97.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx97.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx98.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx98.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx10_wx99.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx10_wx99.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx11_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx11_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx11_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx11_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx12_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx12_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx12_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx12_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx13_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx13_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx13_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx13_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx14_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx14_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx14_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx14_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx15_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx15_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx15_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx15_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx16_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx16_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx17_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx17_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx17_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx17_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx18_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx18_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx18_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx18_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx19_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx19_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx19_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx19_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx1_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx1_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx2_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx2_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx2_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx2_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx3_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx3_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx3_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx3_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx4_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx4_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx4_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx4_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx5_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx5_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx5_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx5_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx6_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx6_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx6_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx6_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx7_wx78.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx7_wx78.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx7_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx7_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx100.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx100.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx101.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx101.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx102.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx102.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx103.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx103.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_bgp.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_bgp.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_lcdc.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_lcdc.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_lyc.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_lyc.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_obp0.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_obp0.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_obp1.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_obp1.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_scx1.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_scx1.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_scx8.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_scx8.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_scy.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_scy.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx71_during_hblank.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx71_during_hblank.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx71.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx71.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx71_wx79_wx_87.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx71_wx79_wx_87.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx72.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx72.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx80_wx71.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wx80_wx71.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wy_ahead_during_hblank.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wy_ahead_during_hblank.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wy_ahead_during_oam_scan.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wy_ahead_during_oam_scan.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wy_behind_during_hblank.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wy_behind_during_hblank.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wy.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_change_wy.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_color3.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_color3.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_disable_bg.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_disable_bg.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_disable_enable_bg.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_disable_enable_bg.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_ly_not_equal_wy.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_ly_not_equal_wy.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_more_lines_b.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_more_lines_b.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_more_lines.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_more_lines.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_never_enable.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_never_enable.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_no_wx_trigger.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_no_wx_trigger.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_postpone_wx_restore.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_postpone_wx_restore.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_reenable_window.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_reenable_window.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_wx134.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_wx134.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_wx135.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_wx135.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_wx16.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_wx16.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_wy12.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_wy12.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx79_wy13.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx79_wy13.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx80_color3.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx80_color3.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx80_ly_not_equal_wy.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx80_ly_not_equal_wy.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx80_more_lines_b.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx80_more_lines_b.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx80_more_lines.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx80_more_lines.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx80_sprite_x72.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx80_sprite_x72.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx80_sprite_x73.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx80_sprite_x73.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx80_sprite_x74.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx80_sprite_x74.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx80_sprite_x75.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx80_sprite_x75.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx81.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx81.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx82.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx82.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx83.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx83.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx84.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx84.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx85.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx85.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx86.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx86.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx87.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx87.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx88.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx88.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx89.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx89.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx90.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx90.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx91.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx91.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx92.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx92.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx93.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx93.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx94.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx94.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx95.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx95.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx96.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx96.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx97.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx97.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx98.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx98.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx8_wx99.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx8_wx99.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx100.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx100.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx101.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx101.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx102.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx102.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx103.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx103.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx81.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx81.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx82.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx82.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx83.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx83.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx84.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx84.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx85.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx85.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx86.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx86.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx87.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx87.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx88.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx88.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx89.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx89.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx90.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx90.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx91.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx91.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx92.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx92.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx93.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx93.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx94.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx94.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx95.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx95.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx96.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx96.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx97.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx97.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx98.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx98.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx0_wx9_wx99.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx0_wx9_wx99.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx100.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx100.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx101.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx101.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx102.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx102.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx103.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx103.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx81.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx81.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx82.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx82.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx83.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx83.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx84.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx84.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx85.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx85.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx86.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx86.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx87.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx87.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx88.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx88.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx89.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx89.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx90.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx90.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx91.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx91.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx92.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx92.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx93.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx93.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx94.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx94.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx95.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx95.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx96.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx96.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx97.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx97.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx98.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx98.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx10_wx99.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx10_wx99.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx100.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx100.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx101.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx101.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx102.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx102.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx103.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx103.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx81.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx81.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx82.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx82.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx83.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx83.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx84.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx84.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx85.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx85.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx86.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx86.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx87.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx87.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx88.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx88.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx89.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx89.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx90.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx90.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx91.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx91.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx92.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx92.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx93.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx93.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx94.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx94.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx95.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx95.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx96.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx96.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx97.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx97.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx98.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx98.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx8_wx99.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx8_wx99.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx100.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx100.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx101.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx101.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx102.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx102.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx103.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx103.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx79.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx79.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx80.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx80.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx81.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx81.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx82.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx82.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx83.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx83.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx84.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx84.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx85.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx85.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx86.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx86.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx87.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx87.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx88.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx88.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx89.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx89.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx90.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx90.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx91.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx91.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx92.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx92.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx93.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx93.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx94.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx94.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx95.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx95.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx96.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx96.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx97.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx97.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx98.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx98.png"},
            F {"docboy/ppu/rendering/window_retrigger_pixel_glitch_scx1_wx9_wx99.gb",
               "docboy/ppu/window_retrigger_pixel_glitch_scx1_wx9_wx99.png"},
            F {"docboy/ppu/rendering/window_trigger_win_disabled_blink_enabled.gb",
               "docboy/ppu/window_trigger_win_disabled_blink_enabled.png"},
            F {"docboy/ppu/rendering/window_trigger_win_disabled.gb", "docboy/ppu/window_trigger_win_disabled.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_ahead_during_hblank.gb",
               "docboy/ppu/window_trigger_wy_ahead_during_hblank.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_ahead_during_oam_scan.gb",
               "docboy/ppu/window_trigger_wy_ahead_during_oam_scan.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_behind_during_hblank.gb",
               "docboy/ppu/window_trigger_wy_behind_during_hblank.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_behind_during_oam_scan.gb",
               "docboy/ppu/window_trigger_wy_behind_during_oam_scan.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_blink_back_during_oam_scan.gb",
               "docboy/ppu/window_trigger_wy_blink_back_during_oam_scan.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_blink_during_hblank_round0.gb",
               "docboy/ppu/window_trigger_wy_blink_during_hblank_round0.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_blink_during_hblank_round1.gb",
               "docboy/ppu/window_trigger_wy_blink_during_hblank_round1.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_blink_during_hblank_round2.gb",
               "docboy/ppu/window_trigger_wy_blink_during_hblank_round2.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_blink_during_next_hblank_round1.gb",
               "docboy/ppu/window_trigger_wy_blink_during_next_hblank_round1.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_blink_during_next_hblank_round2.gb",
               "docboy/ppu/window_trigger_wy_blink_during_next_hblank_round2.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_blink_during_oam_scan.gb",
               "docboy/ppu/window_trigger_wy_blink_during_oam_scan.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_out_of_screen_during_hblank.gb",
               "docboy/ppu/window_trigger_wy_out_of_screen_during_hblank.png"},
            F {"docboy/ppu/rendering/window_trigger_wy_out_of_screen_during_oam_scan.gb",
               "docboy/ppu/window_trigger_wy_out_of_screen_during_oam_scan.png"},
            F {"docboy/ppu/rendering/window_turn_off_during_oam_scan_turn_on.gb",
               "docboy/ppu/window_turn_off_during_oam_scan_turn_on.png"},
            F {"docboy/ppu/rendering/window_turn_off_during_pixel_transfer_turn_on_round1.gb",
               "docboy/ppu/window_turn_off_during_pixel_transfer_turn_on_round1.png"},
            F {"docboy/ppu/rendering/window_turn_off_during_pixel_transfer_turn_on_round2.gb",
               "docboy/ppu/window_turn_off_during_pixel_transfer_turn_on_round2.png"},
            F {"docboy/ppu/rendering/window_wx0_scx0.gb", "docboy/ppu/window_wx0_scx0.png"},
            F {"docboy/ppu/rendering/window_wx0_scx1.gb", "docboy/ppu/window_wx0_scx1.png"},
            F {"docboy/ppu/rendering/window_wx0_scx2.gb", "docboy/ppu/window_wx0_scx2.png"},
            F {"docboy/ppu/rendering/window_wx0_scx3.gb", "docboy/ppu/window_wx0_scx3.png"},
            F {"docboy/ppu/rendering/window_wx0_scx4.gb", "docboy/ppu/window_wx0_scx4.png"},
            F {"docboy/ppu/rendering/window_wx0_scx5.gb", "docboy/ppu/window_wx0_scx5.png"},
            F {"docboy/ppu/rendering/window_wx0_scx6.gb", "docboy/ppu/window_wx0_scx6.png"},
            F {"docboy/ppu/rendering/window_wx0_scx7.gb", "docboy/ppu/window_wx0_scx7.png"},
            F {"docboy/ppu/rendering/window_wx10.gb", "docboy/ppu/window_wx10.png"},
            F {"docboy/ppu/rendering/window_wx11.gb", "docboy/ppu/window_wx11.png"},
            F {"docboy/ppu/rendering/window_wx12.gb", "docboy/ppu/window_wx12.png"},
            F {"docboy/ppu/rendering/window_wx13.gb", "docboy/ppu/window_wx13.png"},
            F {"docboy/ppu/rendering/window_wx14.gb", "docboy/ppu/window_wx14.png"},
            F {"docboy/ppu/rendering/window_wx15.gb", "docboy/ppu/window_wx15.png"},
            F {"docboy/ppu/rendering/window_wx16.gb", "docboy/ppu/window_wx16.png"},
            F {"docboy/ppu/rendering/window_wx1.gb", "docboy/ppu/window_wx1.png"},
            F {"docboy/ppu/rendering/window_wx2.gb", "docboy/ppu/window_wx2.png"},
            F {"docboy/ppu/rendering/window_wx3.gb", "docboy/ppu/window_wx3.png"},
            F {"docboy/ppu/rendering/window_wx4.gb", "docboy/ppu/window_wx4.png"},
            F {"docboy/ppu/rendering/window_wx5.gb", "docboy/ppu/window_wx5.png"},
            F {"docboy/ppu/rendering/window_wx6.gb", "docboy/ppu/window_wx6.png"},
            F {"docboy/ppu/rendering/window_wx7.gb", "docboy/ppu/window_wx7.png"},
            F {"docboy/ppu/rendering/window_wx8.gb", "docboy/ppu/window_wx8.png"},
            F {"docboy/ppu/rendering/window_wx9.gb", "docboy/ppu/window_wx9.png"}, );
    }

#if ALL_TEST_ROMS
    SECTION("mbc") {
        SECTION("mbc1") {
            RUN_TEST_ROMS(S {"mooneye/mbc/mbc1/bits_bank1.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/bits_bank2.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/bits_mode.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/bits_ramg.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/ram_64kb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/ram_256kb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/rom_512kb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/rom_1Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/rom_2Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/rom_4Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/rom_8Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc1/rom_16Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}});
        }

        SECTION("mbc5") {
            RUN_TEST_ROMS(S {"mooneye/mbc/mbc5/rom_512kb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc5/rom_1Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc5/rom_2Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc5/rom_4Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc5/rom_8Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc5/rom_16Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc5/rom_32Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                          S {"mooneye/mbc/mbc5/rom_64Mb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}});
        }
    }

    SECTION("boot") {
        RUN_TEST_ROMS(S {"mooneye/boot_div-dmgABCmgb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/boot_hwio-dmgABCmgb.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/boot_regs-dmgABC.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      F {"docboy/boot/boot_div_phase_round1.gb", "docboy/ok.png"},
                      F {"docboy/boot/boot_div_phase_round2.gb", "docboy/ok.png"},
                      F {"docboy/boot/boot_ppu_phase_round1.gb", "docboy/ok.png"},
                      F {"docboy/boot/boot_ppu_phase_round2.gb", "docboy/ok.png"},
                      F {"docboy/boot/boot_ppu_phase_round3.gb", "docboy/ok.png"},
                      F {"docboy/boot/boot_ppu_phase_round4.gb", "docboy/ok.png"},
                      F {"docboy/boot/boot_stat_lyc_eq_ly_round1.gb", "docboy/ok.png"},
                      F {"docboy/boot/boot_stat_lyc_eq_ly_round2.gb", "docboy/ok.png"},
                      F {"docboy/boot/boot_vram_tile_data.gb", "docboy/ok.png"},
                      F {"docboy/boot/boot_vram_tile_map.gb", "docboy/ok.png"}, );
    }

    SECTION("oam") {
        RUN_TEST_ROMS(S {"mooneye/bits/mem_oam.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}}, );
    }

    SECTION("io") {
        RUN_TEST_ROMS(S {"mooneye/bits/unused_hwio-GS.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}}

        );
    }

    SECTION("timers") {
        RUN_TEST_ROMS(S {"mooneye/timers/div_write.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/rapid_toggle.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tim00.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tim00_div_trigger.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tim01.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tim01_div_trigger.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tim10.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tim10_div_trigger.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tim11.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tim11_div_trigger.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tima_reload.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tima_write_reloading.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/timers/tma_write_reloading.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/div_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      F {"docboy/timers/div_timing_round1.gb", "docboy/ok.png"},
                      F {"docboy/timers/div_timing_round2.gb", "docboy/ok.png"},
                      F {"docboy/timers/timer_interrupt_flag_timing.gb", "docboy/ok.png"},
                      F {"docboy/timers/write_tima_while_reloading_uses_tima.gb", "docboy/ok.png"},
                      F {"docboy/timers/write_tima_while_reloading_aborts_interrupt.gb", "docboy/ok.png"}, );
    }

    SECTION("interrupts") {
        RUN_TEST_ROMS(S {"mooneye/interrupts/ie_push.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/ei_sequence.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/if_ie_registers.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/intr_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/rapid_di_ei.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      F {"docboy/interrupts/joypad_interrupt_timing_manual_ei_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/joypad_interrupt_timing_manual_ei_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/joypad_interrupt_timing_manual_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/joypad_interrupt_timing_manual_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/serial_interrupt_timing_halted_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/serial_interrupt_timing_halted_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/serial_interrupt_timing_manual_ei_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/serial_interrupt_timing_manual_ei_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/serial_interrupt_timing_manual_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/serial_interrupt_timing_manual_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/serial_interrupt_timing_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/serial_interrupt_timing_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx0_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx0_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx1_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx1_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx2_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx2_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx3_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx4_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx4_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx5_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_halted_scx5_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_oam_v1_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_oam_v1_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_oam_v2_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_oam_v2_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_oam_v3_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_oam_v3_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx0_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx0_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx1_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx1_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx2_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx2_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx3_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx3_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx4_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx4_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx5_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/stat_interrupt_timing_scx5_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/timer_interrupt_timing_halted_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/timer_interrupt_timing_halted_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/timer_interrupt_timing_manual_ei_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/timer_interrupt_timing_manual_ei_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/timer_interrupt_timing_manual_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/timer_interrupt_timing_manual_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/timer_interrupt_timing_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/timer_interrupt_timing_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/vblank_interrupt_timing_manual_ei_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/vblank_interrupt_timing_manual_ei_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/vblank_interrupt_timing_manual_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/vblank_interrupt_timing_manual_round2.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/vblank_interrupt_timing_round1.gb", "docboy/ok.png"},
                      F {"docboy/interrupts/vblank_interrupt_timing_round2.gb", "docboy/ok.png"}, );
    }

    SECTION("dma") {
        RUN_TEST_ROMS(S {"mooneye/oam_dma/basic.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/oam_dma/reg_read.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/oam_dma/sources-GS.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/oam_dma_restart.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/oam_dma_start.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      S {"mooneye/oam_dma_timing.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      F {"docboy/dma/dma_during_oam_scan.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_read_vram_cpu_read_rom.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_read_vram_cpu_read_vram.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_read_wram1_cpu_read_oam.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_read_wram1_cpu_read_rom.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_read_wram1_cpu_read_vram.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_read_wram1_cpu_write_hram.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_read_wram1_cpu_write_wram2.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_timing_round1.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_timing_round2.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_transfer.gb", "docboy/ok.png"},
                      F {"docboy/dma/dma_transfer_ff00.gb", "docboy/ok.png"},
                      F {"docboy/dma/rendering/dma_blocks_oam_scan.gb", "docboy/dma/dma_blocks_oam_scan.png"},

        );
    }

    SECTION("bus") {
        RUN_TEST_ROMS(F {"docboy/bus/read_oam_in_pixel_transfer.gb", "docboy/ok.png"},
                      F {"docboy/bus/read_vram_in_pixel_transfer.gb", "docboy/ok.png"});
    }

    SECTION("serial") {
        RUN_TEST_ROMS(F {"mooneye/serial/boot_sclk_align-dmgABCmgb.gb", "mooneye/boot_sclk_align-dmgABCmgb.png"});
    }

    SECTION("integration") {
        RUN_TEST_ROMS(
            F {"hacktix/bully.gb", "hacktix/bully.png"}, F {"hacktix/strikethrough.gb", "hacktix/strikethrough.png"},

            // docboy
            F {"docboy/integration/rendering/change_bgp_from_boot.gb", "docboy/integration/change_bgp_from_boot.png",
               StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_ppu_reset.gb",
               "docboy/integration/change_bgp_from_ppu_reset.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph0_scx0.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph0_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph0_scx1.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph0_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph0_scx2.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph0_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph0_scx3.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph0_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph0_scx4.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph0_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph0_scx5.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph0_scx5.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph0_scx6.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph0_scx6.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph0_scx7.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph0_scx7.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph0_scx8.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph0_scx8.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph1_scx0.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph1_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph1_scx1.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph1_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph1_scx2.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph1_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph1_scx3.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph1_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph1_scx4.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph1_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph1_scx5.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph1_scx5.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph1_scx6.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph1_scx6.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph1_scx7.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph1_scx7.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph1_scx8.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph1_scx8.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph2_scx0.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph2_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph2_scx1.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph2_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph2_scx2.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph2_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph2_scx3.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph2_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph2_scx4.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph2_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph2_scx5.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph2_scx5.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph2_scx6.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph2_scx6.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph2_scx7.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph2_scx7.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph2_scx8.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph2_scx8.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph3_scx0.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph3_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph3_scx1.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph3_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph3_scx2.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph3_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph3_scx3.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph3_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph3_scx4.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph3_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph3_scx5.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph3_scx5.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph3_scx6.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph3_scx6.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph3_scx7.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph3_scx7.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph3_scx8.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph3_scx8.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph4_scx0.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph4_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph4_scx1.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph4_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph4_scx2.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph4_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph4_scx3.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph4_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph4_scx4.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph4_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph4_scx5.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph4_scx5.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph4_scx6.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph4_scx6.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph4_scx7.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph4_scx7.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph4_scx8.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph4_scx8.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph5_scx0.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph5_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph5_scx1.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph5_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph5_scx2.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph5_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph5_scx3.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph5_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph5_scx4.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph5_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph5_scx5.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph5_scx5.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph5_scx6.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph5_scx6.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph5_scx7.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph5_scx7.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph5_scx8.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph5_scx8.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph6_scx0.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph6_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph6_scx1.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph6_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph6_scx2.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph6_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph6_scx3.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph6_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph6_scx4.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph6_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph6_scx5.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph6_scx5.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph6_scx6.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph6_scx6.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph6_scx7.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph6_scx7.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph6_scx8.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph6_scx8.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph7_scx0.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph7_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph7_scx1.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph7_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph7_scx2.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph7_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph7_scx3.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph7_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph7_scx4.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph7_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph7_scx5.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph7_scx5.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph7_scx6.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph7_scx6.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph7_scx7.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph7_scx7.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph7_scx8.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph7_scx8.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph8_scx0.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph8_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph8_scx1.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph8_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph8_scx2.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph8_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph8_scx3.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph8_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph8_scx4.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph8_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph8_scx5.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph8_scx5.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph8_scx6.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph8_scx6.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph8_scx7.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph8_scx7.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_read_stat_hblank_mode_ph8_scx8.gb",
               "docboy/integration/change_bgp_from_read_stat_hblank_mode_ph8_scx8.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_hblank_interrupt_halted_scx0.gb",
               "docboy/integration/change_bgp_from_stat_hblank_interrupt_halted_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_hblank_interrupt_halted_scx1.gb",
               "docboy/integration/change_bgp_from_stat_hblank_interrupt_halted_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_hblank_interrupt_halted_scx2.gb",
               "docboy/integration/change_bgp_from_stat_hblank_interrupt_halted_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_hblank_interrupt_halted_scx3.gb",
               "docboy/integration/change_bgp_from_stat_hblank_interrupt_halted_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_hblank_interrupt_halted_scx4.gb",
               "docboy/integration/change_bgp_from_stat_hblank_interrupt_halted_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_hblank_interrupt_scx0.gb",
               "docboy/integration/change_bgp_from_stat_hblank_interrupt_scx0.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_hblank_interrupt_scx1.gb",
               "docboy/integration/change_bgp_from_stat_hblank_interrupt_scx1.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_hblank_interrupt_scx2.gb",
               "docboy/integration/change_bgp_from_stat_hblank_interrupt_scx2.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_hblank_interrupt_scx3.gb",
               "docboy/integration/change_bgp_from_stat_hblank_interrupt_scx3.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_hblank_interrupt_scx4.gb",
               "docboy/integration/change_bgp_from_stat_hblank_interrupt_scx4.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_oam_interrupt.gb",
               "docboy/integration/change_bgp_from_stat_oam_interrupt.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_oam_interrupt_halted.gb",
               "docboy/integration/change_bgp_from_stat_oam_interrupt_halted.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_vblank_interrupt.gb",
               "docboy/integration/change_bgp_from_stat_vblank_interrupt.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_stat_vblank_interrupt_halted.gb",
               "docboy/integration/change_bgp_from_stat_vblank_interrupt_halted.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_vblank_interrupt.gb",
               "docboy/integration/change_bgp_from_vblank_interrupt.png", StopAtInstruction {0x40}},
            F {"docboy/integration/rendering/change_bgp_from_vblank_interrupt_halted.gb",
               "docboy/integration/change_bgp_from_vblank_interrupt_halted.png", StopAtInstruction {0x40}}, );
    }
#endif

#endif

#if MEALYBUG_ONLY_TEST_ROMS
    SECTION("mealybug") {
        RUN_TEST_ROMS(F {"mealybug/m2_win_en_toggle.gb", "mealybug/m2_win_en_toggle.png", GREY_PALETTE},
                      F {"mealybug/m3_bgp_change.gb", "mealybug/m3_bgp_change.png", GREY_PALETTE},
                      F {"mealybug/m3_bgp_change_sprites.gb", "mealybug/m3_bgp_change_sprites.png", GREY_PALETTE},
                      // F {"mealybug/m3_lcdc_bg_en_change.gb", "mealybug/m3_lcdc_bg_en_change.png", GREY_PALETTE},
                      //                                        F{"mealybug/m3_lcdc_bg_map_change.gb",
                      //                "mealybug/m3_lcdc_bg_map_change.png", GREY_PALETTE},
                      //                F{"mealybug/m3_lcdc_obj_en_change.gb", "mealybug/m3_lcdc_obj_en_change.png",
                      //                GREY_PALETTE}, F{"mealybug/m3_lcdc_obj_en_change_variant.gb",
                      //                "mealybug/m3_lcdc_obj_en_change_variant.png", GREY_PALETTE},
                      //                F{"mealybug/m3_lcdc_obj_size_change.gb", "mealybug/m3_lcdc_obj_size_change.png",
                      //                GREY_PALETTE}, F{"mealybug/m3_lcdc_obj_size_change_scx.gb",
                      //                "mealybug/m3_lcdc_obj_size_change_scx.png", GREY_PALETTE},
                      //                F{"mealybug/m3_lcdc_tile_sel_change.gb", "mealybug/m3_lcdc_tile_sel_change.png",
                      //                GREY_PALETTE}, F{"mealybug/m3_lcdc_tile_sel_win_change.gb",
                      //                "mealybug/m3_lcdc_tile_sel_win_change.png", GREY_PALETTE},
                      //                F{"mealybug/m3_lcdc_win_en_change_multiple.gb",
                      //                "mealybug/m3_lcdc_win_en_change_multiple.png", GREY_PALETTE},
                      //                F{"mealybug/m3_lcdc_win_en_change_multiple_wx.gb",
                      //                "mealybug/m3_lcdc_win_en_change_multiple_wx.png", GREY_PALETTE},
                      //                F{"mealybug/m3_lcdc_win_map_change.gb", "mealybug/m3_lcdc_win_map_change.png",
                      //                GREY_PALETTE}, F{"mealybug/m3_obp0_change.gb", "mealybug/m3_obp0_change.png",
                      //                GREY_PALETTE},
                      F {"mealybug/m3_scx_high_5_bits.gb", "mealybug/m3_scx_high_5_bits.png", GREY_PALETTE},
                      F {"mealybug/m3_scx_low_3_bits.gb", "mealybug/m3_scx_low_3_bits.png", GREY_PALETTE},
                      // F{"mealybug/m3_scy_change.gb", "mealybug/m3_scy_change.png", GREY_PALETTE},
                      F {"mealybug/m3_window_timing.gb", "mealybug/m3_window_timing.png", GREY_PALETTE},
                      F {"mealybug/m3_window_timing_wx_0.gb", "mealybug/m3_window_timing_wx_0.png", GREY_PALETTE},
                      F {"mealybug/m3_wx_4_change.gb", "mealybug/m3_wx_4_change.png", GREY_PALETTE},
                      F {"mealybug/m3_wx_4_change_sprites.gb", "mealybug/m3_wx_4_change_sprites.png", GREY_PALETTE},
                      F {"mealybug/m3_wx_5_change.gb", "mealybug/m3_wx_5_change.png", GREY_PALETTE},
                      F {"mealybug/m3_wx_6_change.gb", "mealybug/m3_wx_6_change.png", GREY_PALETTE}, );
    }
#endif

#if WIP_ONLY_TEST_ROMS
    SECTION("wip") {
        RUN_TEST_ROMS(F {"mealybug/m3_wx_4_change_sprites.gb", "mealybug/m3_wx_4_change_sprites.png", GREY_PALETTE},
                      // F{"mealybug/m3_wx_5_change.gb", "mealybug/m3_wx_5_change.png", GREY_PALETTE},
                      //                F{"mealybug/m3_wx_6_change.gb", "mealybug/m3_wx_6_change.png", GREY_PALETTE},
        );
    }
#endif
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return 1;

    IMG_Init(IMG_INIT_PNG);

    return Catch::Session().run(argc, argv);
}