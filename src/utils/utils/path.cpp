#include "path.h"
#include "exceptions.hpp"
#include "os.h"
#include "strings.hpp"
#include <iostream>
#include <iterator>

#ifdef WINDOWS
#include "Windows.h"
#endif

path::path(const std::string& path) {
    check(!path.empty(), "cannot build an empty path");
    split(path, std::back_inserter(parts), SEP);
    isAbsolute = path[0] == SEP;
}

std::string path::string() const {
    std::string s = join(parts, SEP);
    if (isAbsolute)
        return SEP + s;
    return s;
}

path path::with_extension(const std::string& extension) const {
    return path {*this}.replace_extension(extension);
}

path& path::replace_extension(const std::string& extension) {
    std::string& lastPart = parts.back();
    std::size_t dot = lastPart.find_last_of('.');
    if (dot != std::string::npos) {
        lastPart.replace(dot + 1, std::string::npos, extension);
    }

    return *this;
}

path& path::operator/(const std::string& part) {
    parts.push_back(part);
    return *this;
}

path& path::operator/(const path& part) {
    check(!part.isAbsolute, "cannot append absolute path");
    parts.insert(parts.end(), part.parts.begin(), part.parts.end());
    return *this;
}

std::ostream& operator<<(std::ostream& os, const path& path) {
    os << path.filename();
    return os;
}

std::string path::filename() const {
    return parts.back();
}

path operator/(const path& p1, const path& p2) {
    path p {p1};
    p = p / p2;
    return p;
}

path temp_directory_path() {
    // Implementation inspired by boost::temp_directory_path
    std::string temp_path;

#ifdef POSIX
    const char* env_tmp;
    (env_tmp = std::getenv("TMPDIR")) || (env_tmp = std::getenv("TMP")) || (env_tmp = std::getenv("TEMP")) ||
        (env_tmp = std::getenv("TEMPDIR"));

#ifdef __ANDROID__
    const char* default_tmp = "/data/local/tmp";
#else
    static const char* default_tmp = "/tmp";
#endif
    temp_path = env_tmp ? env_tmp : default_tmp;

#else // Windows
    CHAR temp_path_buf[MAX_PATH];
    if (GetTempPath(MAX_PATH, temp_path_buf) == 0) {
        fatal("failed to obtain a temporary directory");
    }
    temp_path = temp_path_buf;

#endif

    return path(temp_path);
}