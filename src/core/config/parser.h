#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include "config.h"

class ConfigParser {
public:
    static Config parse(const std::string &filename, bool *ok = nullptr, std::string *error = nullptr);
};

#endif // CONFIGPARSER_H