#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <string>
#include <vector>

template<typename T = uint8_t>
std::vector<T> read_file(const std::string &filename, bool *ok = nullptr);

template<>
std::vector<uint8_t> read_file<uint8_t>(const std::string &filename, bool *ok);

bool write_file(const std::string &filename, void *data, long length);

#include "fileutils.tpp"


#endif // FILEUTILS_H