#include "container.h"

namespace Tui {
Tui::Container::Container(Node::Type type) :
    Node(type) {
}

void Tui::Container::addNode(std::unique_ptr<Node>&& node) {
    children.emplace_back(std::move(node));
}
} // namespace Tui