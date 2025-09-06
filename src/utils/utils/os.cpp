#include "utils/os.h"

#include <sys/stat.h>

#ifdef WINDOWS
#include "Windows.h"
#endif

// Ensure these are defined for compatibility with Windows.

#ifndef __S_IFMT
#define __S_IFMT 0170000 /* These bits determine file type.  */
#endif

#ifndef __S_IFDIR
#define __S_IFDIR 0040000 /* Directory.  */
#endif

#ifndef __S_IFREG
#define __S_IFREG 0100000 /* Regular file.  */
#endif

#ifndef __S_ISTYPE
#define __S_ISTYPE(mode, mask) (((mode)&__S_IFMT) == (mask))
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) __S_ISTYPE((mode), __S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode) __S_ISTYPE((mode), __S_IFREG)
#endif

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
#ifdef WINDOWS
    return CreateDirectory(filename.c_str(), nullptr);
#else
    return mkdir(filename.c_str(), mode) == 0;
#endif
}

#ifndef WINDOWS
DirectoryTraverser iterate_directory(const std::string& filename) {
    return DirectoryTraverser {filename};
}

RecursiveDirectoryTraverser recursive_iterate_directory(const std::string& filename) {
    return RecursiveDirectoryTraverser {filename};
}
#endif