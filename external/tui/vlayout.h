#ifndef VLAYOUT_H
#define VLAYOUT_H

#include "container.h"
#include "node.h"
#include <vector>

namespace Tui {
struct VLayout : Container {
    explicit VLayout() :
        Container(Node::Type::VLayout) {
    }
};
} // namespace Tui

#endif // VLAYOUT_H