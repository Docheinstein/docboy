#ifndef BLOCK_H
#define BLOCK_H

#include "node.h"
#include "text.h"
#include <memory>
#include <optional>

namespace Tui {
struct Block : Node {
    explicit Block(std::optional<uint32_t> width = std::nullopt);

    Block& operator<<(const Text& text);
    Block& operator<<(Block& (*manip)(Block&));

    std::vector<Text> lines;
    std::optional<uint32_t> width;
};

Block& endl(Block&);

// Helpers for std::unique_ptr
Block& operator<<(const std::unique_ptr<Block>& block, const Text& text);
Block& operator<<(const std::unique_ptr<Block>& block, Block& (*manip)(const std::unique_ptr<Block>&));
Block& endl(const std::unique_ptr<Block>&);
} // namespace Tui

#endif // BLOCK_H