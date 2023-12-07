#ifndef TEXT_H
#define TEXT_H

#include "token.h"
#include "traits.h"
#include <optional>
#include <string>
#include <vector>

namespace Tui {
class Text {
public:
    using Length = Explicit<uint32_t, struct LengthTag>;
    using RawLength = Explicit<uint32_t, struct RawLengthTag>;
    using Index = Explicit<uint32_t, struct IndexTag>;
    using RawIndex = Explicit<uint32_t, struct RawIndexTag>;

    Text();
    Text(const Token& token);
    Text(Token&& token);

    template <typename T, typename = std::enable_if_t<std::negation_v<std::disjunction<
                              std::is_same<std::decay_t<T>, Text>, std::is_same<std::decay_t<T>, Token>>>>>
    Text(T&& value) {
        std::string s;

        if constexpr (std::is_arithmetic_v<std::decay_t<T>>) {
            s = std::to_string(value);
        } else {
            s = std::forward<T>(value);
        }

        tokens.reserve(s.size());
        for (const auto& c : s) {
            tokens.emplace_back(c);
        }
        length = s.size();
    }

    Text& operator=(const Token& token);
    Text& operator=(Token&& token);

    Text& operator+=(const Token& token);
    Text& operator+=(Token&& token);
    Text& operator+=(const Text& text);

    friend Text operator+(const Text& text1, const Text& text2);

    [[nodiscard]] std::string str() const;
    [[nodiscard]] Length size() const;
    [[nodiscard]] Text substr(RawIndex start) const;
    [[nodiscard]] Text substr(RawIndex start, RawLength len) const;
    [[nodiscard]] Text substr(RawIndex start, Length len) const;
    [[nodiscard]] Text rpad(Length len, char ch = ' ') const;
    [[nodiscard]] Text lpad(Length len, char ch = ' ') const;
    [[nodiscard]] std::optional<RawIndex> find(char ch, RawIndex pos = RawIndex {0},
                                               RawLength len = RawLength {UINT32_MAX}) const;

protected:
    std::vector<Token> tokens;
    Length length {};
};
} // namespace Tui
#endif // TEXT_H