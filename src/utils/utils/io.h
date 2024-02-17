#ifndef IO_H
#define IO_H

#include <cstdint>
#include <string>
#include <vector>

std::vector<uint8_t> read_file(const std::string& filename, bool* ok = nullptr);
void write_file(const std::string& filename, const void* data, std::size_t length, bool* ok = nullptr);

std::vector<std::string> read_file_lines(const std::string& filename, bool* ok = nullptr);
void write_file_lines(const std::string& filename, const std::vector<std::string>& lines, bool* ok = nullptr);

#endif // IO_H