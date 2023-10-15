#ifndef PATH_H
#define PATH_H

#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <vector>

class path {
public:
    explicit path(const std::string& path);

    path& operator/(const std::string& part);
    path& operator/(const path& part);
    friend std::ostream& operator<<(std::ostream& os, const path& path);

    [[nodiscard]] std::string filename() const;

    path& replace_extension(const std::string& extension);

    [[nodiscard]] std::string string() const;

private:
    static inline const char SEP = '/';

    std::vector<std::string> parts {};
    bool isAbsolute {};
};

path temp_directory_path();

#endif // PATH_H