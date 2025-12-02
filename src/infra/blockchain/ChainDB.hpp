#pragma once
#include <mutex>
#include <string>
#include <vector>

#include "Block.hpp"
#include "DBFile.hpp"
#include "IChainRepo.hpp"
#include "IConfig.hpp"
#include "ICrypto.hpp"

namespace blockchain
{
class ChainDB : public IChainRepo
{
private:
    std::shared_ptr<db::DBFile> db;
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::mutex mutex;

public:
    ChainDB(const std::shared_ptr<db::DBFile>& db,
            const std::shared_ptr<config::IConfig>& config,
            const std::shared_ptr<crypto::ICrypto>& crypto);

    void init() override;

    bool insertBlock(const Block& block) override;
    bool findBlockByHash(const std::string& hash, Block& block) override;
    void getBlocksByIndexRange(u_int start,
                               u_int count,
                               const std::string& lastHash,
                               std::vector<Block>& outBlocks) override;
    u_int countBlocksAfterHash(const std::string& hash) override;
    bool hasBlock(const std::string& hash) override;
    void loadAllBlocks(std::vector<Block>& blocks) override;

    bool findTip(Block& block) override;
    bool findTipIndex(u_int& index) override;
};
}  // namespace blockchain
