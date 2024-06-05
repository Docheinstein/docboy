#include <optional>

template<typename T, typename P>
void IniReader::add_property(const std::string &name, T &option, P&& parser) {
    properties.emplace(name, &option);

    // clang-format off
    parsers.emplace(name, std::function<bool(const std::string&, void*)> {
            [&parser](const std::string& string_value, void* target_option) {
        // We can safely cast void* to T* here.
        T* typed_target_option = static_cast<T*>(target_option);

        if (std::optional<T> value = parser(string_value); value) {
            // Option parsed successfully
            *typed_target_option = *value;
            return true;
        }

        return false;
    }});
    // clang-format on
}
