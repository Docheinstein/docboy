#ifndef MENUITEM_H
#define MENUITEM_H

struct MenuItem {
    enum class Justification { Left, Center, Right };

    MenuItem() = default;
    explicit MenuItem(std::string&& t) :
        text {std::move(t)} {
    }

    std::string text {};
    bool selectable {true};
    bool visible {true};
    Justification justification {Justification::Left};
    std::function<void()> on_enter {};
    std::function<void()> on_prev {};
    std::function<void()> on_next {};
};

#endif // MENUITEM_H
