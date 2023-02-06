#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <string>
#include <vector>

std::vector<uint8_t> readfile(const std::string &filename, bool *ok = nullptr);

#endif // FILEUTILS_H