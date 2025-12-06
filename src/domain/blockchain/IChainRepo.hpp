#pragma once
#include <string>
#include <vector>

#include "Block.hpp"
#include "Message.hpp"

namespace blockchain
{
using u_int = unsigned int;

class IChainRepo
{
public:
    virtual ~IChainRepo() = default;

    virtual void init() = 0;

    virtual bool insertBlock(const Block& block) = 0;
    virtual bool findBlockByHash(const std::string& hash, Block& block) = 0;
    virtual void getBlocksByIndexRange(u_int start,
                                       u_int count,
                                       const std::string& lastHash,
                                       std::vector<Block>& outBlocks) = 0;
    virtual u_int countBlocksAfterHash(const std::string& hash) = 0;
    virtual bool hasBlock(const std::string& hash) = 0;
    virtual void loadAllBlocks(std::vector<Block>& blocks) = 0;

    virtual bool findTip(Block& block) = 0;
    virtual bool findTipIndex(u_int& index) = 0;
};
}  // namespace blockchain
