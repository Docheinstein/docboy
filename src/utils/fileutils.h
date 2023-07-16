#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <cstdint>
#include <string>
#include <vector>

template <typename T = uint8_t>
std::vector<T> read_file(const std::string& filename, bool* ok = nullptr);

template <>
std::vector<uint8_t> read_file<uint8_t>(const std::string& filename, bool* ok);

std::vector<std::string> read_file_lines(const std::string& filename, bool* ok = nullptr);

void write_file(const std::string& filename, void* data, long length, bool* ok = nullptr);

void write_file_lines(const std::string& filename, const std::vector<std::string>& lines, bool* ok = nullptr);

#include "fileutils.tpp"

#endif // FILEUTILS_H