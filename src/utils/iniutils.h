#ifndef INIUTILS_H
#define INIUTILS_H

#include <map>
#include <string>

using INISection = std::map<std::string, std::string>;
using INI = std::map<std::string, INISection>;

INI ini_read(const std::string& filename, bool* ok = nullptr, std::string* error = nullptr);
void ini_write(const std::string& filename, const INI& ini, bool* ok = nullptr, std::string* error = nullptr);

#endif // INIUTILS_H
