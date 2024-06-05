#include "utils/path.h"

#include <iostream>
#include <iterator>

#include "utils/exceptions.h"
#include "utils/os.h"
#include "utils/strings.h"

#ifdef WINDOWS
#include "Windows.h"
#endif

Path::Path(const std::string& path) {
    ASSERT(!path.empty(), "cannot build an empty Path");
    split(path, std::back_inserter(parts), SEP);
    is_absolute = path[0] == SEP;
}

std::string Path::string() const {
    std::string s = join(parts, SEP);
    if (is_absolute) {
        return SEP + s;
    }
    return s;
}

Path Path::with_extension(const std::string& extension) const {
    return Path {*this}.replace_extension(extension);
}

Path& Path::replace_extension(const std::string& extension) {
    std::string& last_part = parts.back();
    std::size_t dot = last_part.find_last_of('.');
    if (dot != std::string::npos) {
        last_part.replace(dot + 1, std::string::npos, extension);
    }

    return *this;
}

Path& Path::operator/(const std::string& part) {
    parts.push_back(part);
    return *this;
}

Path& Path::operator/(const Path& part) {
    ASSERT(!part.is_absolute, "cannot append absolute Path");
    parts.insert(parts.end(), part.parts.begin(), part.parts.end());
    return *this;
}

std::ostream& operator<<(std::ostream& os, const Path& path) {
    os << path.filename();
    return os;
}

std::string Path::filename() const {
    return parts.back();
}

Path operator/(const Path& p1, const Path& p2) {
    Path p {p1};
    p = p / p2;
    return p;
}

Path temp_directory_path() {
    // Implementation inspired by boost::temp_directory_Path
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
    CHAR temp_Path_buf[MAX_PATH];
    if (GetTempPath(MAX_PATH, temp_Path_buf) == 0) {
        FATAL("failed to obtain a temporary directory");
    }
    temp_Path = temp_Path_buf;

#endif

    return Path {temp_path};
}