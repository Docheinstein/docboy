#ifndef UTILSOS_H
#define UTILSOS_H

#include <cstdint>
#include <string>

#include "traversal.h"

uint64_t file_size(const std::string& filename);

bool file_exists(const std::string& filename);
bool is_file(const std::string& filename);
bool is_directory(const std::string& filename);

bool create_directory(const std::string& filename, int mode = 0755);

#ifndef WINDOWS
DirectoryTraverser iterate_directory(const std::string& filename);
RecursiveDirectoryTraverser recursive_iterate_directory(const std::string& filename);
#endif

#endif // UTILSOS_H