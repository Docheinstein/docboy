#ifndef TRAITS_H
#define TRAITS_H

namespace Tui {
template <typename T, typename Tag>
struct Explicit {
    explicit Explicit() :
        value() {
    }
    explicit Explicit(T value) :
        value(value) {
    }
    operator T() const {
        return value;
    }
    Explicit<T, Tag>& operator=(const T& val) {
        value = val;
        return *this;
    }

    T value;
};
} // namespace Tui

#endif // TRAITS_H