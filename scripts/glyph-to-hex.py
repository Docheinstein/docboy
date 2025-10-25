# Modify `glyph` as you wish and re-run script.
glyph = """\
10000000\
11000000\
11100000\
11110000\
11100000\
11000000\
10000000\
00000000\
"""


def glyph_to_hex():
    assert(len(glyph) == 64)
    h = 0
    for c in glyph:
        assert(c == "0" or c == "1")
        h = h << 1
        if c == "1":
            h = h | 1

    return h


if __name__ == "__main__":
    print(hex(glyph_to_hex()))
