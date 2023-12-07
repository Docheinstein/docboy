#ifndef NODE_H
#define NODE_H

namespace Tui {
struct Node {
    enum class Type { Block, HLayout, VLayout, Divider };

    explicit Node(Type type) :
        type(type) {
    }

    Type type;
};
} // namespace Tui

#endif // NODE_H