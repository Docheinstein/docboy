#ifndef PATH_H
#define PATH_H

#include <string>
#include <vector>

class path {
public:
    path() = default;
    explicit path(const std::string& path);

    path& operator/(const std::string& part);
    path& operator/(const path& part);

    friend path operator/(const path& p1, const path& p2);
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