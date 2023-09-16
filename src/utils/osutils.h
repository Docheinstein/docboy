#ifndef OSUTILS_H
#define OSUTILS_H

#include <string>
#include <sys/stat.h>

typedef decltype(stat::st_size) file_size_t;

file_size_t file_size(const std::string& filename);

bool file_exists(const std::string& filename);
bool is_file(const std::string& filename);
bool is_directory(const std::string& filename);

#endif // OSUTILS_H