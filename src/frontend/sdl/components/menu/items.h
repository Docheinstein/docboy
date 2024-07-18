#ifndef MENUITEMS_H
#define MENUITEMS_H

#include <functional>
#include <string>

#include "components/menu/item.h"

struct Label : MenuItem {
    explicit Label(std::string&& t, MenuItem::Justification justification_ = MenuItem::Justification::Left) :
        MenuItem {std::move(t)} {
        selectable = false;
        justification = justification_;
    }
};

struct Button : MenuItem {
    Button(std::string&& t, const std::function<void()>& on_enter_fn,
           MenuItem::Justification justification_ = MenuItem::Justification::Left) :
        MenuItem {std::move(t)} {
        on_enter = on_enter_fn;
        justification = justification_;
    }
};

struct Spinner : MenuItem {
    explicit Spinner(const std::function<void()>& on_change_fn,
                     MenuItem::Justification justification_ = MenuItem::Justification::Left) :
        MenuItem {} {
        on_prev = on_next = on_change_fn;
        justification = justification_;
    }
    Spinner(const std::function<void()>& on_prev_fn, const std::function<void()>& on_next_fn,
            MenuItem::Justification justification_ = MenuItem::Justification::Left) :
        MenuItem {} {
        on_prev = on_prev_fn;
        on_next = on_next_fn;
        justification = justification_;
    }
};

#endif // MENUITEMS_H
