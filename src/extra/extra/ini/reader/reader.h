#ifndef INIREADER_H
#define INIREADER_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

class IniReader {
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
    void addProperty(const std::string& name, T& option, P&& parser);

    void addProperty(const std::string& name, std::string& option);

    void addCommentPrefix(const std::string& prefix);

private:
    std::map<std::string, void*> properties {};
    std::map<std::string, std::function<bool(const std::string&, void*)>> parsers {};

    std::vector<std::string> commentPrefixes {};
};

#include "reader.tpp"

#endif // INIREADER_H
