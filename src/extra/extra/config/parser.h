#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

class ConfigParser {
public:
    struct Result {
        enum class Outcome {
            Success,
            ErrorReadFailed,
            ErrorParseFailed,
        };

        Outcome outcome {};
        uint32_t lastReadLine {};

        explicit operator bool() const {
            return outcome == Outcome::Success;
        }
    };

    Result parse(const std::string& filename);

    template <typename T, typename P>
    void addOption(const std::string& name, T& option, P&& parser);

    void addOption(const std::string& name, std::string& option);

    void addCommentPrefix(const std::string& prefix);

private:
    std::map<std::string, void*> options {};
    std::map<std::string, std::function<bool(const std::string&, void*)>> parsers {};

    std::vector<std::string> commentPrefixes {};
};

#include "parser.tpp"

#endif // CONFIGPARSER_H
