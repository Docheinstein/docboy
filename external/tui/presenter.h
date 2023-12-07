#ifndef PRESENTER_H
#define PRESENTER_H

#include "node.h"
#include <memory>
#include <ostream>

namespace Tui {
class Presenter {
public:
    explicit Presenter(std::ostream& os);

    void present(const Node& node);

private:
    std::ostream& os;
};
} // namespace Tui

#endif // PRESENTER_H