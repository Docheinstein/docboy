#include "fileutils.h"
#include <fstream>
#include <filesystem>
#include <iostream>

std::vector<uint8_t> readfile(const std::string &filename, bool *ok) {
    std::vector<uint8_t> out;

    std::ifstream ifs(filename);
    if (!ifs) {
        if (ok)
            *ok = false;
        return out;
    }

    std::streamsize size;
    try {
        size = static_cast<std::streamsize>(std::filesystem::file_size(filename));
    } catch (std::filesystem::filesystem_error &err) {
        if (ok)
            *ok = false;
        return out;
    }

    try {
        out.resize(size);
        ifs.read((char *) out.data(), size);
    } catch (std::ifstream::failure &err) {
        if (ok)
            *ok = false;
        return out;
    }

    if (ok)
        *ok = true;
    return out;
}
