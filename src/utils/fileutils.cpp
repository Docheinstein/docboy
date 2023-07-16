#include "fileutils.h"
#include <iostream>

bool write_file(const std::string& filename, void* data, long length) {
    std::ofstream ofs(filename);
    if (!ofs) {
        return false;
    }

    ofs.write(static_cast<const char*>(data), length);
    return ofs.good();
}

template <>
std::vector<uint8_t> read_file<uint8_t>(const std::string& filename, bool* ok) {
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
    } catch (std::filesystem::filesystem_error& err) {
        if (ok)
            *ok = false;
        return out;
    }

    try {
        out.resize(size);
        ifs.read((char*)out.data(), size);
    } catch (std::ifstream::failure& err) {
        if (ok)
            *ok = false;
        return out;
    }

    if (ok)
        *ok = true;
    return out;
}

std::vector<std::string> read_file_lines(const std::string& filename, bool* ok) {
    std::vector<std::string> lines;
    std::ifstream ifs(filename);
    if (!ifs.is_open() || ifs.fail()) {
        if (ok)
            *ok = false;
        return lines;
    }

    std::string line;
    while (getline(ifs, line))
        lines.push_back(line);

    if (ok)
        *ok = true;
    return lines;
}
