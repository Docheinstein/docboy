#define CATCH_CONFIG_RUNNER

#include "catch2/catch.hpp"
#include "docboy/core/core.h"
#include "docboy/gameboy/gameboy.h"
#include "extra/serial/endpoints/buffer.h"
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

#define TABLE_1(t1, data) GENERATE(table<t1> data)
#define TABLE_2(t1, t2, data) GENERATE(table<t1, t2> data)
#define TABLE_3(t1, t2, t3, data) GENERATE(table<t1, t2, t3> data)
#define TABLE_4(t1, t2, t3, t4, data) GENERATE(table<t1, t2, t3, t4> data)

#define TABLE_PICKER(_1, _2, _3, _4, _5, FUNC, ...) FUNC
#define TABLE(...) TABLE_PICKER(__VA_ARGS__, TABLE_4, TABLE_3, TABLE_2, TABLE_1)(__VA_ARGS__)

static const std::string TEST_ROMS_PATH = "tests/roms/";
static const std::string TEST_RESULTS_PATH = "tests/results/";

static constexpr uint32_t FRAMEBUFFER_NUM_PIXELS = Specs::Display::WIDTH * Specs::Display::HEIGHT;
static constexpr uint32_t FRAMEBUFFER_SIZE = sizeof(uint16_t) * FRAMEBUFFER_NUM_PIXELS;

using Palette = std::vector<uint16_t>;

static const Palette DEFAULT_PALETTE {};
static const Palette GREY_PALETTE {0xFFFF, 0xAD55, 0x52AA, 0x0000}; // {0xFF, 0xAA, 0x55, 0x00} in RGB565

static constexpr uint64_t DURATION_VERY_LONG = 250'000'000;
static constexpr uint64_t DURATION_LONG = 100'000'000;
static constexpr uint64_t DURATION_MEDIUM = 30'000'000;
static constexpr uint64_t DURATION_SHORT = 5'000'000;
static constexpr uint64_t DURATION_VERY_SHORT = 1'500'000;

static constexpr uint64_t DEFAULT_DURATION = DURATION_VERY_LONG;

static void load_png(const std::string& filename, void* buffer, int format = -1, uint32_t* size = nullptr) {
    SDL_Surface* surface {};
    SDL_Surface* convertedSurface {};

    surface = IMG_Load(filename.c_str());
    if (!surface)
        goto error;

    if (format != -1) {
        convertedSurface = SDL_ConvertSurfaceFormat(surface, format, 0);
        if (!convertedSurface)
            goto error;
        surface = convertedSurface;
        convertedSurface = nullptr;
    }

    if (size)
        *size = surface->h * surface->pitch;

    memcpy(buffer, surface->pixels, surface->h * surface->pitch);
    goto cleanup;

error:
    if (size)
        *size = 0;

cleanup:
    if (surface)
        SDL_FreeSurface(surface);
}

[[maybe_unused]] static void dump_png_framebuffer(const std::string& filename) {
    uint16_t buffer[FRAMEBUFFER_NUM_PIXELS];
    load_png(filename, buffer, SDL_PIXELFORMAT_RGB565);
    std::cout << hexdump(buffer, FRAMEBUFFER_NUM_PIXELS) << std::endl;
}

static bool save_framebuffer_as_png(const std::string& filename, const uint16_t* buffer) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        (void*)buffer, Specs::Display::WIDTH, Specs::Display::HEIGHT, SDL_BITSPERPIXEL(SDL_PIXELFORMAT_RGB565),
        Specs::Display::WIDTH * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGB565), SDL_PIXELFORMAT_RGB565);

    int result = -1;
    if (surface) {
        result = IMG_SavePNG(surface, filename.c_str());
    }

    SDL_FreeSurface(surface);

    return result == 0;
}

static inline uint16_t convert_framebuffer_pixel(uint16_t p, const Palette& pixelsConversionMap) {
    for (uint8_t c = 0; c < 4; c++)
        if (p == Lcd::RGB565_PALETTE[c])
            return pixelsConversionMap[c];
    checkNoEntry();
    return p;
}

static void convert_framebuffer_pixels(uint16_t* dst, const uint16_t* src, const Palette& pixelsConversionMap) {
    for (uint32_t i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
        dst[i] = convert_framebuffer_pixel(src[i], pixelsConversionMap);
    }
}

static bool are_framebuffer_equals(const uint16_t* buf1, const uint16_t* buf2) {
    return memcmp(buf1, buf2, FRAMEBUFFER_SIZE) == 0;
}

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
        checkIntervalTicks_ = ticks;
        return static_cast<RunnerImpl&>(*this);
    }

    bool run() {
        auto* impl = static_cast<RunnerImpl*>(this);
        impl->onRun();

        bool hasEverChecked {false};

        for (tick = 1; tick <= maxTicks_; tick++) {
            core.tick();
            if (impl->shouldCheckExpectation()) {
                hasEverChecked = true;
                if (impl->checkExpectation()) {
                    return true;
                }
            }
        }

        if (hasEverChecked) {
            impl->onExpectationFailed();
        } else {
            UNSCOPED_INFO("Expectation never checked!");
        }

        return false;
    }

    GameBoy gb;
    Core core {gb};

protected:
    std::string romName; // debug
    uint64_t tick {};
    uint64_t maxTicks_ {UINT64_MAX};
    uint64_t checkIntervalTicks_ {UINT64_MAX};
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
        if (tick % checkIntervalTicks_ == 0) {
            pendingCheck = true;
        }

        if (pendingCheck && keep_bits<2>(core.gb.video.STAT) == 1) {
            pendingCheck = false;
            return true;
        }

        return false;
    }

    bool checkExpectation() {
        // Compare framebuffer only in VBlank
        check(keep_bits<2>(core.gb.video.STAT) == 1);
        memcpy(lastFramebuffer, gb.lcd.getPixels(), FRAMEBUFFER_SIZE);
        if (!pixelColors.empty())
            convert_framebuffer_pixels(lastFramebuffer, gb.lcd.getPixels(), pixelColors);
        return are_framebuffer_equals(lastFramebuffer, expectedFramebuffer);
    }

    void onExpectationFailed() {
        // Framebuffer not equals: figure out where's the (first) problem
        UNSCOPED_INFO("=== " << romName << " ===");

        uint32_t i;
        for (i = 0; i < FRAMEBUFFER_NUM_PIXELS; i++) {
            if (lastFramebuffer[i] != expectedFramebuffer[i]) {
                UNSCOPED_INFO("Framebuffer mismatch at position " << i << ": (actual) 0x" << hex(lastFramebuffer[i])
                                                                  << " != (expected) 0x"
                                                                  << hex(expectedFramebuffer[i]));
                break;
            }
        }
        check(i < FRAMEBUFFER_NUM_PIXELS);

        // Dump framebuffers
        const auto pathActual = (temp_directory_path() / "framebuffer-actual.png").string();
        const auto pathExpected = (temp_directory_path() / "framebuffer-expected.png").string();
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
        return tick % checkIntervalTicks_ == 0;
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
    SECTION("State") {
        std::vector<uint8_t> data1;

        {
            Runner runner;
            runner.rom("blargg/cpu_instrs.gb").maxTicks(10'000).run();
            data1.resize(runner.core.getStateSaveSize());
            runner.core.saveState(data1.data());
        }

        std::vector<uint8_t> data2(data1.size());

        {
            Runner runner;
            runner.rom("blargg/cpu_instrs.gb");
            runner.core.loadState(data1.data());
            runner.core.saveState(data2.data());
        }

        REQUIRE(memcmp(data1.data(), data2.data(), data1.size()) == 0);
    }
}

static bool run_expecting_framebuffer(const std::string& rom, const std::string& result, const Palette& palette) {
    return FramebufferRunner()
        .rom(TEST_ROMS_PATH + rom)
        .maxTicks(DEFAULT_DURATION)
        .checkIntervalTicks(DURATION_VERY_SHORT)
        .expectFramebuffer(TEST_RESULTS_PATH + result, palette)
        .run();
}

static bool run_expecting_serial(const std::string& rom, const std::vector<uint8_t>& result) {
    return SerialRunner()
        .rom(TEST_ROMS_PATH + rom)
        .maxTicks(DEFAULT_DURATION)
        .checkIntervalTicks(DURATION_VERY_SHORT)
        .expectOutput(result)
        .run();
}

TEST_CASE("emulation", "[emulation]") {
    SECTION("mbc") {
        SECTION("mbc1") {
            const auto [rom, expectedResult, palette] =
                TABLE(std::string, std::string, Palette,
                      ({{"mooneye/mbc/mbc1/bits_bank1.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/bits_bank2.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/bits_mode.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/bits_ramg.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/ram_64kb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/ram_256kb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/rom_512kb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/rom_1Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/rom_2Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/rom_4Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/rom_8Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc1/rom_16Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE}}));
            REQUIRE(run_expecting_framebuffer(rom, expectedResult, palette));
        }

        SECTION("mbc5") {
            const auto [rom, expectedResult, palette] =
                TABLE(std::string, std::string, Palette,
                      ({{"mooneye/mbc/mbc5/rom_512kb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc5/rom_1Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc5/rom_2Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc5/rom_4Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc5/rom_8Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc5/rom_16Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc5/rom_32Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE},
                        {"mooneye/mbc/mbc5/rom_64Mb.gb", "mooneye/ok.png", DEFAULT_PALETTE}}));
            REQUIRE(run_expecting_framebuffer(rom, expectedResult, palette));
        }
    }

    SECTION("cpu") {
        const auto [rom, expectedResult, palette] =
            TABLE(std::string, std::string, Palette,
                  ({
                      {"blargg/cpu_instrs.gb", "blargg/cpu_instrs.png", DEFAULT_PALETTE},
                      {"blargg/instr_timing.gb", "blargg/instr_timing.png", DEFAULT_PALETTE},
                  }));
        REQUIRE(run_expecting_framebuffer(rom, expectedResult, palette));
    }

    SECTION("ppu") {
        SECTION("dmg-acid2") {
            const auto [rom, expectedResult, palette] =
                TABLE(std::string, std::string, Palette,
                      ({
                          {"dmg-acid2/dmg-acid2.gb", "dmg-acid2/dmg-acid2.png", GREY_PALETTE},
                      }));
            REQUIRE(run_expecting_framebuffer(rom, expectedResult, palette));
        }
    }

    SECTION("timers") {
        const auto [rom, expectedResult] =
            TABLE(std::string, std::vector<uint8_t>,
                  ({
                      {"mooneye/timers/div_write.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/rapid_toggle.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tim00.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tim00_div_trigger.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tim01.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tim01_div_trigger.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tim10.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tim10_div_trigger.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tim11.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tim11_div_trigger.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tima_reload.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tima_write_reloading.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                      {"mooneye/timers/tma_write_reloading.gb", {0x03, 0x05, 0x08, 0x0D, 0x15, 0x22}},
                  }));
        REQUIRE(run_expecting_serial(rom, expectedResult));
    }

#ifdef MEALYBUG
    SECTION("ppu disabled") {
        SECTION("mealybug") {
            const auto [rom, expectedResult, maxTicks, palette] =
                TABLE(std::string, std::string, uint64_t, Palette,
                      ({
                          {"mealybug/m2_win_en_toggle.gb", "mealybug/m2_win_en_toggle.png", DURATION_VERY_SHORT,
                           GREY_PALETTE},
                          //                            {"mealybug/m3_bgp_change.gb",
                          //                            "mealybug/m3_bgp_change.png", 1000000},
                          //                            {"mealybug/m3_bgp_change_sprites.gb",
                          //                            "mealybug/m3_bgp_change_sprites.png", 1000000},
                          //                            {"mealybug/m3_lcdc_bg_en_change2.gb",
                          //                            "mealybug/m3_lcdc_bg_en_change2.png", 1000000},
                          //                            {"mealybug/m3_lcdc_bg_en_change.gb",
                          //                            "mealybug/m3_lcdc_bg_en_change.png", 1000000},
                          //                            {"mealybug/m3_lcdc_bg_map_change2.gb",
                          //                            "mealybug/m3_lcdc_bg_map_change2.png", 1000000},
                          //                            {"mealybug/m3_lcdc_bg_map_change.gb",
                          //                            "mealybug/m3_lcdc_bg_map_change.png", 1000000},
                          //                            {"mealybug/m3_lcdc_obj_en_change.gb",
                          //                            "mealybug/m3_lcdc_obj_en_change.png", 1000000},
                          //                            {"mealybug/m3_lcdc_obj_en_change_variant.gb",
                          //                            "mealybug/m3_lcdc_obj_en_change_variant.png", 1000000},
                          //                            {"mealybug/m3_lcdc_obj_size_change.gb",
                          //                            "mealybug/m3_lcdc_obj_size_change.png", 1000000},
                          //                            {"mealybug/m3_lcdc_obj_size_change_scx.gb",
                          //                            "mealybug/m3_lcdc_obj_size_change_scx.png", 1000000},
                          //                            {"mealybug/m3_lcdc_tile_sel_change2.gb",
                          //                            "mealybug/m3_lcdc_tile_sel_change2.png", 1000000},
                          //                            {"mealybug/m3_lcdc_tile_sel_change.gb",
                          //                            "mealybug/m3_lcdc_tile_sel_change.png", 1000000},
                          //                            {"mealybug/m3_lcdc_tile_sel_win_change2.gb",
                          //                            "mealybug/m3_lcdc_tile_sel_win_change2.png", 1000000},
                          //                            {"mealybug/m3_lcdc_tile_sel_win_change.gb",
                          //                            "mealybug/m3_lcdc_tile_sel_win_change.png", 1000000},
                          //                            {"mealybug/m3_lcdc_win_en_change_multiple.gb",
                          //                            "mealybug/m3_lcdc_win_en_change_multiple.png", 1000000},
                          //                            {"mealybug/m3_lcdc_win_en_change_multiple_wx.gb",
                          //                            "mealybug/m3_lcdc_win_en_change_multiple_wx.png",
                          //                            1000000},
                          //                            {"mealybug/m3_lcdc_win_map_change2.gb",
                          //                            "mealybug/m3_lcdc_win_map_change2.png", 1000000},
                          //                            {"mealybug/m3_lcdc_win_map_change.gb",
                          //                            "mealybug/m3_lcdc_win_map_change.png", 1000000},
                          //                            {"mealybug/m3_obp0_change.gb",
                          //                            "mealybug/m3_obp0_change.png", 1000000},
                          //                            {"mealybug/m3_scx_high_5_bits_change2.gb",
                          //                            "mealybug/m3_scx_high_5_bits_change2.png", 1000000},
                          //                            {"mealybug/m3_scx_high_5_bits.gb",
                          //                            "mealybug/m3_scx_high_5_bits.png", 1000000},
                          //                            {"mealybug/m3_scx_low_3_bits.gb",
                          //                            "mealybug/m3_scx_low_3_bits.png", 1000000},
                          //                            {"mealybug/m3_scy_change2.gb",
                          //                            "mealybug/m3_scy_change2.png", 1000000},
                          //                            {"mealybug/m3_scy_change.gb",
                          //                            "mealybug/m3_scy_change.png", 1000000},
                          //                            {"mealybug/m3_window_timing.gb",
                          //                            "mealybug/m3_window_timing.png", 1000000},
                          //                            {"mealybug/m3_window_timing_wx_0.gb",
                          //                            "mealybug/m3_window_timing_wx_0.png", 1000000},
                          //                            {"mealybug/m3_wx_4_change.gb",
                          //                            "mealybug/m3_wx_4_change.png", 1000000},
                          //                            {"mealybug/m3_wx_4_change_sprites.gb",
                          //                            "mealybug/m3_wx_4_change_sprites.png", 1000000},
                          //                            {"mealybug/m3_wx_5_change.gb",
                          //                            "mealybug/m3_wx_5_change.png", 1000000},
                          //                            {"mealybug/m3_wx_6_change.gb",
                          //                            "mealybug/m3_wx_6_change.png", 1000000},
                      }));
            REQUIRE(Runner()
                        .rom(rom)
                        .expectFramebuffer(expectedResult)
                        .pixelsColor(palette)
                        .maxTicks(maxTicks)
                        .framebufferCheckTicksInterval(DURATION_VERY_SHORT)
                        .run()
                        .areFramebufferEquals);
        }
    }
#endif
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return 1;

    IMG_Init(IMG_INIT_PNG);

    return Catch::Session().run(argc, argv);
}