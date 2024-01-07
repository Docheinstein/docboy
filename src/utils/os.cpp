#include "os.h"
#include <sys/stat.h>

uint64_t file_size(const std::string& filename) {
    struct stat st {};
    if (stat(filename.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        return 0;
    }

    return st.st_size;
}

bool file_exists(const std::string& filename) {
    struct stat st {};
    return stat(filename.c_str(), &st) == 0;
}

bool is_file(const std::string& filename) {
    struct stat st {};
    return stat(filename.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool is_directory(const std::string& filename) {
    struct stat st {};
    return stat(filename.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool create_directory(const std::string& filename, int mode) {
    return mkdir(filename.c_str(), mode) == 0;
}