#pragma once
#include <sqlite3.h>

#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace db
{
class DBFile
{
private:
    std::string path;
    sqlite3* db = nullptr;
    std::mutex mutex;

    void bindParams(sqlite3_stmt* stmt, const std::vector<std::string>& params);

public:
    explicit DBFile(std::string dbPath);
    ~DBFile();

    void open();
    void close();
    void exec(const std::string& sql);
    void select(const std::string& sql,
                const std::function<void(const std::vector<std::string>&)>& callback);
    bool executePrepared(const std::string& sql, const std::vector<std::string>& params);
    bool isOpen() const;
};
}  // namespace db
