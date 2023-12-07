#ifndef HLAYOUT_H
#define HLAYOUT_H

#include "container.h"
#include "node.h"
#include <vector>

namespace Tui {
struct HLayout : Container {
    explicit HLayout() :
        Container(Node::Type::HLayout) {
    }
};
} // namespace Tui
#endif // HLAYOUT_H