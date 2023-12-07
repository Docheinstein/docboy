#include "block.h"

namespace Tui {
Block::Block(std::optional<uint32_t> width) :
    Node(Node::Type::Block),
    width(width) {
}

Block& Tui::Block::operator<<(const Text& text) {
    // No lines yet: add one
    if (lines.empty())
        lines.emplace_back();

    // Split text in lines by \n
    Text::RawIndex i {0};

    std::optional<Text::RawIndex> newLine;
    do {
        newLine = text.find('\n', i);
        if (newLine) {
            lines.back() += text.substr(i, Text::RawLength {*newLine - i});
            lines.emplace_back();
            i = Text::Index {*newLine + 1};
        }
    } while (newLine);

    lines.back() += text.substr(i);

    return *this;
}

Block& Tui::Block::operator<<(Block& (*manip)(Block&)) {
    return manip(*this);
}

Block& endl(Block& b) {
    b.lines.emplace_back();
    return b;
}

Block& operator<<(const std::unique_ptr<Block>& block, const Text& text) {
    return *block << text;
}

Block& operator<<(const std::unique_ptr<Block>& block, Block& (*manip)(const std::unique_ptr<Block>&)) {
    return manip(block);
}

Block& endl(const std::unique_ptr<Block>& b) {
    return endl(*b);
}

} // namespace Tui