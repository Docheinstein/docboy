#ifndef INIWRITER_H
#define INIWRITER_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

class IniWriter {
public:
    bool write(const std::map<std::string, std::string>& properties, const std::string& filename) const;
};

#endif // INIWRITER_H
