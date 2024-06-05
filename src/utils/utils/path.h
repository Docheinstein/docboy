#ifndef UTILSPATH_H
#define UTILSPATH_H

#include <string>
#include <vector>

class Path {
public:
    Path() = default;
    explicit Path(const std::string& p);

    Path& operator/(const std::string& part);
    Path& operator/(const Path& part);

    friend Path operator/(const Path& p1, const Path& p2);
    friend std::ostream& operator<<(std::ostream& os, const Path& p);

    std::string filename() const;

    Path with_extension(const std::string& extension) const;
    Path& replace_extension(const std::string& extension);

    std::string string() const;

private:
    static inline const char SEP = '/';

    std::vector<std::string> parts {};
    bool is_absolute {};
};

Path temp_directory_path();

#endif // UTILSPATH_H