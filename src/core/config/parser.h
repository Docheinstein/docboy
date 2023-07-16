#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include "config.h"
#include <memory>

class ISectionParser;

class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser();

    Config parse(const std::string& filename, bool* ok = nullptr, std::string* error = nullptr);

private:
    std::unique_ptr<ISectionParser> sectionParser;
};

#endif // CONFIGPARSER_H