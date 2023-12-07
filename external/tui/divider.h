#ifndef DIVIDER_H
#define DIVIDER_H

#include "node.h"
#include "text.h"

namespace Tui {
struct Divider : Node {
    explicit Divider(Text&& text) :
        Node(Node::Type::Divider),
        text(std::move(text)) {
    }

    Text text;
};
} // namespace Tui
#endif // DIVIDER_H