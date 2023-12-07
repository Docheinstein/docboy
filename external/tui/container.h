#ifndef CONTAINER_H
#define CONTAINER_H

#include "node.h"
#include <memory>
#include <vector>

namespace Tui {
struct Container : Node {
    explicit Container(Node::Type type);

    void addNode(std::unique_ptr<Node>&& node);

    std::vector<std::unique_ptr<Node>> children;
};
} // namespace Tui
#endif // CONTAINER_H