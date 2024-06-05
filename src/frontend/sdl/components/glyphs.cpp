#include "components/glyphs.h"

#include "SDL3/SDL.h"

namespace {
constexpr Glyph ASCII_GLYPHS[128] = {
    0x0000000000000000, // 0
    0x0000000000000000, // 1
    0x0000000000000000, // 2
    0x0000000000000000, // 3
    0x0000000000000000, // 4
    0x0000000000000000, // 5
    0x0000000000000000, // 6
    0x0000000000000000, // 7
    0x0000000000000000, // 8
    0x0000000000000000, // 9
    0x0000000000000000, // 10
    0x0000000000000000, // 11
    0x0000000000000000, // 12
    0x0000000000000000, // 13
    0x0000000000000000, // 14
    0x0000000000000000, // 15
    0x0000000000000000, // 16
    0x0000000000000000, // 17
    0x0000000000000000, // 18
    0x0000000000000000, // 19
    0x0000000000000000, // 20
    0x0000000000000000, // 21
    0x0000000000000000, // 22
    0x0000000000000000, // 23
    0x0000000000000000, // 24
    0x0000000000000000, // 25
    0x0000000000000000, // 26
    0x0000000000000000, // 27
    0x0000000000000000, // 28
    0x0000000000000000, // 29
    0x0000000000000000, // 30
    0x0000000000000000, // 31
    0x0000000000000000, // 32 ( )
    0x183C3C1818001800, // 33 (!)
    0x6C6C000000000000, // 34 (")
    0x6C6CFE6CFE6C6C00, // 35 (#)
    0x307CC0780CF83000, // 36 ($)
    0x00C6CC183066C600, // 37 (%)
    0x386C3876DCCC7600, // 38 (&)
    0x6060C00000000000, // 39 (')
    0x1830606060301800, // 40 (()
    0x6030181818306000, // 41 ())
    0x00663CFF3C660000, // 42 (*)
    0x003030FC30300000, // 43 (+)
    0x0000000000303060, // 44 (,)
    0x000000FC00000000, // 45 (-)
    0x0000000000303000, // 46 (.)
    0x060C183060C08000, // 47 (/)
    0x7CC6CEDEF6E67C00, // 48 (0)
    0x307030303030FC00, // 49 (1)
    0x78CC0C3860CCFC00, // 50 (2)
    0x78CC0C380CCC7800, // 51 (3)
    0x1C3C6CCCFE0C1E00, // 52 (4)
    0xFCC0F80C0CCC7800, // 53 (5)
    0x3860C0F8CCCC7800, // 54 (6)
    0xFCCC0C1830303000, // 55 (7)
    0x78CCCC78CCCC7800, // 56 (8)
    0x78CCCC7C0C187000, // 57 (9)
    0x0030300000303000, // 58 (:)
    0x0030300000303060, // 59 (;)
    0x183060C060301800, // 60 (<)
    0x0000FC0000FC0000, // 61 (=)
    0x6030180C18306000, // 62 (>)
    0x78CC0C1830003000, // 63 (?)
    0x7CC6DEDEDEC07800, // 64 (@)
    0x3078CCCCFCCCCC00, // 65 (A)
    0xFC66667C6666FC00, // 66 (B)
    0x3C66C0C0C0663C00, // 67 (C)
    0xF86C6666666CF800, // 68 (D)
    0xFE6268786862FE00, // 69 (E)
    0xFE6268786860F000, // 70 (F)
    0x3C66C0C0CE663E00, // 71 (G)
    0xCCCCCCFCCCCCCC00, // 72 (H)
    0x7830303030307800, // 73 (I)
    0x1E0C0C0CCCCC7800, // 74 (J)
    0xE6666C786C66E600, // 75 (K)
    0xF06060606266FE00, // 76 (L)
    0xC6EEFEFED6C6C600, // 77 (M)
    0xC6E6F6DECEC6C600, // 78 (N)
    0x386CC6C6C66C3800, // 79 (O)
    0xFC66667C6060F000, // 80 (P)
    0x78CCCCCCDC781C00, // 81 (Q)
    0xFC66667C6C66E600, // 82 (R)
    0x78CCE0701CCC7800, // 83 (S)
    0xFCB4303030307800, // 84 (T)
    0xCCCCCCCCCCCCFC00, // 85 (U)
    0xCCCCCCCCCC783000, // 86 (V)
    0xC6C6C6D6FEEEC600, // 87 (W)
    0xC6C66C38386CC600, // 88 (X)
    0xCCCCCC7830307800, // 89 (Y)
    0xFEC68C183266FE00, // 90 (Z)
    0x7860606060607800, // 91 ([)
    0xC06030180C060200, // 92 (\)
    0x7818181818187800, // 93 (])
    0x10386CC600000000, // 94 (^)
    0x00000000000000FF, // 95 (_)
    0x3030180000000000, // 96 (`)
    0x0000780C7CCC7600, // 97 (a)
    0xE060607C6666DC00, // 98 (b)
    0x000078CCC0CC7800, // 99 (c)
    0x1C0C0C7CCCCC7600, // 100 (d)
    0x000078CCFCC07800, // 101 (e)
    0x386C60F06060F000, // 102 (f)
    0x000076CCCC7C0CF8, // 103 (g)
    0xE0606C766666E600, // 104 (h)
    0x3000703030307800, // 105 (i)
    0x0C000C0C0CCCCC78, // 106 (j)
    0xE060666C786CE600, // 107 (k)
    0x7030303030307800, // 108 (l)
    0x0000CCFEFED6C600, // 109 (m)
    0x0000F8CCCCCCCC00, // 110 (n)
    0x000078CCCCCC7800, // 111 (o)
    0x0000DC66667C60F0, // 112 (p)
    0x000076CCCC7C0C1E, // 113 (q)
    0x0000DC766660F000, // 114 (r)
    0x00007CC0780CF800, // 115 (s)
    0x10307C3030341800, // 116 (t)
    0x0000CCCCCCCC7600, // 117 (u)
    0x0000CCCCCC783000, // 118 (v)
    0x0000C6D6FEFE6C00, // 119 (w)
    0x0000C66C386CC600, // 120 (x)
    0x0000CCCCCC7C0CF8, // 121 (y)
    0x0000FC983064FC00, // 122 (z)
    0x1C3030E030301C00, // 123 ({)
    0x1818180018181800, // 124 (|)
    0xE030301C3030E000, // 125 (})
    0x76DC000000000000, // 126 (~)
    0x0000000000000000, // 127
};

/*
 * Draw a glyph of a certain color to the given RGBA buffer.
 * e.g. to draw a glyph on the second letter of a buffer of three letters:
 * start  = 1
 * stride = 3
 *
 * <----------stride---------->
 *    [0]      [1]      [2]
 * <-start->
 * +--------+--------+--------+
 * |        |    *   |        |
 * |        |  *  *  |        |
 * |        | *    * |        |
 * |        | ****** |        |
 * |        | *    * |        |
 * |        | *    * |        |
 * +--------+--------+--------+
 */
void glyph_to_rgba(Glyph glyph, uint32_t color, uint32_t* data, uint32_t x, uint32_t y, uint32_t stride) {
    for (int r = 0; r < GLYPH_HEIGHT; r++) {
        for (int c = 0; c < GLYPH_WIDTH; c++) {
            auto offset = (r + y) * stride + x + c;
            data[offset] = (glyph & 0x8000000000000000) ? color : 0;
            glyph = glyph << 1;
        }
    }
}
} // namespace

Glyph char_to_glyph(char c) {
    return ASCII_GLYPHS[static_cast<uint8_t>(c < 0 ? 0 : c)];
}

void draw_glyph(SDL_Texture* texture, Glyph glyph, uint32_t x, uint32_t y, uint32_t color, uint32_t stride) {
    uint32_t* buffer;
    int pitch;

    SDL_LockTexture(texture, nullptr, (void**)&buffer, &pitch);
    draw_glyph(buffer, glyph, x, y, color, stride);
    SDL_UnlockTexture(texture);
}

void draw_glyph(uint32_t* buffer, Glyph glyph, uint32_t x, uint32_t y, uint32_t color, uint32_t stride) {
    glyph_to_rgba(glyph, color, buffer, x, y, stride);
}