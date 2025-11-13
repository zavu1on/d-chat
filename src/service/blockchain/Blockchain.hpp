#pragma once
#include <vector>

#include "Block.hpp"

namespace blockchain
{
class Blockchain
{
public:
    Blockchain() = default;
    const std::vector<Block>& chain() const noexcept { return m_chain; }

    // Adds block if previousHash matches last hash (or chain empty). Computes timestamp &
    // block.hash internally.
    void addBlock(Block b);

    bool verifyChain() const;

private:
    std::vector<Block> m_chain;
};
}  // namespace blockchain