#include <optional>

template<typename T, typename P>
void ConfigParser::addOption(const std::string &name, T &option, P&& parser) {
    options.emplace(name, &option);

    // clang-format off
    parsers.emplace(name, std::function<bool(const std::string&, void*)> {
            [&parser](const std::string& stringValue, void* targetOption) {
        // We can safely cast void* to T* here.
        T* typedTargetOption = static_cast<T*>(targetOption);

        if (std::optional<T> value = parser(stringValue); value) {
            // Option parsed successfully
            *typedTargetOption = *value;
            return true;
        }

        return false;
    }});
    // clang-format on
}
