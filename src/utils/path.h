#ifndef PATH_H
#define PATH_H

#include <cstdint>
#include <string>
#include <sys/stat.h>

class path {
public:
    explicit path(std::string path);
    explicit path(const char* path);

    path& operator/(const std::string& part);
    friend std::ostream& operator<<(std::ostream& os, const path& path);

    path& replace_extension(const std::string& extension);

    [[nodiscard]] std::string string() const;
    [[nodiscard]] const char* c_str() const;

private:
    std::string path_string;
};

path temp_directory_path();

#endif // PATH_H