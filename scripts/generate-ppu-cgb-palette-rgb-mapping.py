import sys
from pathlib import Path

"""
Script for generate the all the possible RGB565 colors for the CGB pixels.

usage: python generate-ppu-cgb-palette-rgb-mapping <output>
"""

"""
CGB pixel color format:
 7    6    5    4    3    2    1    0    |   7     6   5    4    3    2    1    0
 X   B[4] B[3] B[2] B[1] B[0] G[4] G[3]  |  G[2] G[1] G[0] [R4] R[3] R[2] R[1] R[0]
"""


if __name__ == "__main__":
	with Path(sys.argv[1]).open("w") as f:
		rgb565s = []
		for pixel in range(2**15):
			r5 = (pixel & 0x001F)
			assert(r5 < 32)

			g5 = (pixel & 0x03E0) >> 5
			assert(g5 < 32)

			b5 = (pixel & 0x7C00) >> 10
			assert(b5 < 32)

			g6 = int(round(g5 * 63 / 31))
			rgb565 = r5 << 11 | g6 << 5 | b5
			rgb565s.append(rgb565)

		array = "{" + ",\n".join([str(p) for p in rgb565s]) + "}"
		file = f"""\
#ifndef COLORMAP_H
#define COLORMAP_H

#include <cstdint>

constexpr uint16_t CGB_PALETTE[]{array};

#endif // COLORMAP_H
"""
		f.write(f"{file}\n")


