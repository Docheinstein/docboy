import sys
from pathlib import Path

"""
Script for compare lines of two files (useful for compare traces).

usage: python compare-files.py <file1> <file2>
"""


def colored(s):
	return "\033[31m" + s + "\033[0m"


def lines_diff(l1, l2):
	s = ""
	for i, c in enumerate(l1):
		if i < len(l2):
			if l2[i] == c:
				s += c
			else:
				s += colored(c)
	return s


def main():
	f1 = sys.argv[1]
	f2 = sys.argv[2]
	with Path(f1).open() as f:
		c1 = f.readlines()
	with Path(f2).open() as f:
		c2 = f.readlines()

	length = min([len(c1), len(c2)])

	for i in range(length):
		if c1[i] != c2[i]:
			print(f"Mismatch at line: {i + 1}")
			print(f"{f1}")
			for j in range(i - 2, i + 2):
				try:
					print(f"{'->' if j == i else '  '} {j}: {lines_diff(c1[j], c2[j])}", end="")
				except:
					pass
			print("----------------")
			print(f"{f2}")
			for j in range(i - 2, i + 2):
				try:
					print(f"{'->' if j == i else '  '} {j}: {lines_diff(c2[j], c1[j])}", end="")
				except:
					pass
			return

	print(f"Matched {length} lines")


if __name__ == "__main__":
	main()