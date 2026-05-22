#ifndef UTILSSTD_H
#define UTILSSTD_H

template <typename Container, typename Predicate>
void erase_if(Container& container, Predicate predicate) {
    // TODO: use std::erase_if when upgrading to C++20.
    for (auto it = container.begin(); it != container.end();) {
        if (predicate(*it)) {
            it = container.erase(it);
        } else {
            ++it;
        }
    }
}

#endif // UTILSSTD_H