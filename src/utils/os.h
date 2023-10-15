#ifndef OS_H
#define OS_H

#include <cstdint>
#include <string>
#include <sys/stat.h>

uint64_t file_size(const std::string& filename);

bool file_exists(const std::string& filename);
bool is_file(const std::string& filename);
bool is_directory(const std::string& filename);

#endif // OS_H