#include <filesystem>
#include <fstream>

template <typename T>
std::vector<T> read_file(const std::string& filename, bool* ok) {
    std::vector<T> out;

    std::ifstream ifs(filename);
    if (!ifs) {
        if (ok)
            *ok = false;
        return out;
    }

    std::streamsize size;
    try {
        size = static_cast<std::streamsize>(std::filesystem::file_size(filename));
    } catch (std::filesystem::filesystem_error& err) {
        if (ok)
            *ok = false;
        return out;
    }

    try {
        out.reserve(size / sizeof(T));
    } catch (std::ifstream::failure& err) {
        if (ok)
            *ok = false;
        return out;
    }

    T value;
    while (ifs.read(reinterpret_cast<char*>(&value), sizeof(value))) {
        out.push_back(value);
    }

    if (ok)
        *ok = true;
    return out;
}
