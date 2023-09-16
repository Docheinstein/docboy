#include "ioutils.h"

void write_file(const std::string& filename, void* data, size_t length, bool* ok) {
    std::ofstream ofs(filename);
    if (!ofs) {
        if (ok)
            *ok = false;
        return;
    }

    ofs.write(static_cast<const char*>(data), static_cast<std::streamsize>(length));

    if (ok)
        *ok = ofs.good();
}

void write_file_lines(const std::string& filename, const std::vector<std::string>& lines, bool* ok) {
    std::ofstream ofs(filename);
    if (!ofs) {
        if (ok)
            *ok = false;
        return;
    }

    for (const auto& line : lines)
        ofs << line << "\n";

    if (ok)
        *ok = ofs.good();
}

#include <iostream>

template <>
std::vector<uint8_t> read_file<uint8_t>(const std::string& filename, bool* ok) {
    std::vector<uint8_t> out;

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

    out.resize(size);

    __try {
        ifs.read((char*)out.data(), static_cast<std::streamsize>(size));
    }
    __catch(std::ifstream::failure & err) {
        std::cout << "4err" << std::endl;

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