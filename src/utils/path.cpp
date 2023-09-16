#include "path.h"
#include "exceptionutils.h"
#include "osutils.h"

path::path(std::string path) :
    path_string(std::move(path)) {
    // TODO: sanitize trailing slash
}

path::path(const char* path) :
    path_string(path) {
    // TODO: sanitize trailing slash
}

std::string path::string() const {
    return path_string;
}

const char* path::c_str() const {
    return path_string.c_str();
}

path& path::replace_extension(const std::string& extension) {
    std::size_t dot = path_string.find_last_of('.');
    if (dot != std::string::npos) {
        path_string.replace(dot + 1, std::string::npos, extension);
    }

    return *this;
}

path& path::operator/(const std::string& part) {
    // TODO cross platform
    path_string += "/" + part;
    return *this;
}

std::ostream& operator<<(std::ostream& os, const path& path) {
    os << path.path_string;
    return os;
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
        THROW(std::runtime_error("failed to obtain a temporary directory"));
    }
    temp_path = temp_path_buf;

#endif

    return path(temp_path);
}