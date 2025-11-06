#include "Blockchain.hpp"

#include <chrono>

void Blockchain::addBlock(Block b)
{
    if (!m_chain.empty())
    {
        b.previousHash = m_chain.back().hash;
    }
    else
    {
        b.previousHash = "0";
    }
    b.timestamp = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
    b.computeHash();
    m_chain.emplace_back(std::move(b));
}

bool Blockchain::verifyChain() const
{
    for (size_t i = 0; i < m_chain.size(); ++i)
    {
        Block tmp = m_chain[i];
        tmp.computeHash();
        if (tmp.hash != m_chain[i].hash) return false;
        if (i > 0 && m_chain[i].previousHash != m_chain[i - 1].hash) return false;
    }
    return true;
}
