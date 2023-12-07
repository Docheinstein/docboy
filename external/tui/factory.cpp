#include "factory.h"
#include "block.h"
#include "divider.h"
#include "hlayout.h"
#include "text.h"
#include "vlayout.h"

namespace Tui {
std::unique_ptr<Block> make_block(std::optional<uint32_t> width) {
    return std::make_unique<Block>(width);
}

std::unique_ptr<HLayout> make_horizontal_layout() {
    return std::make_unique<HLayout>();
}

std::unique_ptr<VLayout> make_vertical_layout() {
    return std::make_unique<VLayout>();
}

std::unique_ptr<Divider> make_divider(Text&& text) {
    return std::make_unique<Divider>(std::move(text));
}
} // namespace Tui