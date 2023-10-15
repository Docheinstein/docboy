#include "os.h"

bool is_file(const std::string& filename) {
    struct stat st {};
    return stat(filename.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool is_directory(const std::string& filename) {
    struct stat st {};
    return stat(filename.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

uint64_t file_size(const std::string& filename) {
    struct stat st {};
    if (stat(filename.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        return 0;
    }

    return st.st_size;
}
