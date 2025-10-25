import sys
from pathlib import Path

"""
Script for generate the all the possible mappings between a 16 bit value
and the array of 8 pixels (4 color each) that would be produced by the PPU.

usage: python generate-ppu-pixel-mapping.py
"""

def encode(h, l):
	row = []
	for b in range(0, 8):
		row.append((h & 1) << 1 | (l & 1))
		l = l >> 1
		h = h >> 1

	return "{" + ",".join([str(p) for p in reversed(row)]) + "}"


if __name__ == "__main__":
	out = ""
	for H in range(0, 2**8):
		for L in range(0, 2 ** 8):
			out += f"{encode(H, L)},\n"
	print(out)
