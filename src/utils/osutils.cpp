#include "osutils.h"
#include <cstdint>
#include <sys/stat.h>

// Definitions taken from <sys/stat.h>

static constexpr uint32_t STAT_IFMT = 0170000;
static constexpr uint32_t STAT_IFDIR = 0020000;
static constexpr uint32_t STAT_IFREG = 0100000;

static inline bool is_file(const struct stat& st) {
    return (st.st_mode & STAT_IFMT) == STAT_IFREG;
}

static inline bool is_directory(const struct stat& st) {
    return (st.st_mode & STAT_IFMT) == STAT_IFDIR;
}

file_size_t file_size(const std::string& filename) {
    struct stat st {};
    if (stat(filename.c_str(), &st) != 0 || !is_file(st)) {
        return 0;
    }

    return st.st_size;
}

bool is_file(const std::string& filename) {
    struct stat st {};
    return stat(filename.c_str(), &st) == 0 && is_file(st);
}

bool is_directory(const std::string& filename) {
    struct stat st {};
    return stat(filename.c_str(), &st) == 0 && is_directory(st);
}