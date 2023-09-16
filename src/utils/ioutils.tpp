#include "exceptionutils.h"
#include <fstream>
#include "osutils.h"

template <typename T>
std::vector<T> read_file(const std::string& filename, bool* ok) {
    std::vector<T> out;

    std::ifstream ifs(filename);
    if (!ifs) {
        if (ok)
            *ok = false;
        return out;
    }

    file_size_t size = file_size(filename);
    if (!size) {
        if (ok)
            *ok = false;
        return out;
    }

    out.reserve(size / sizeof(T));

    T value;
    __try {
        while (ifs.read(reinterpret_cast<char*>(&value), sizeof(value))) {
            out.push_back(value);
        }
    }
    __catch(std::ifstream::failure & err) {
        if (ok)
            *ok = false;
        return out;
    }

    if (ok)
        *ok = true;
    return out;
}
