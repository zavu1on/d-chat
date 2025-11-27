#include "DBFile.hpp"

#include <iostream>

namespace db
{
DBFile::DBFile(std::string dbPath) : path(dbPath) {}

DBFile::~DBFile() { close(); }

void DBFile::open()
{
    std::lock_guard<std::mutex> lock(mutex);

    if (db) return;

    int rc = sqlite3_open(path.c_str(), &db);
    if (rc != SQLITE_OK)
    {
        std::string msg = sqlite3_errmsg(db);
        sqlite3_close(db);
        db = nullptr;
        throw std::runtime_error("DBFile: failed to open database: " + msg);
    }

    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
}

void DBFile::close()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (db)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

void DBFile::exec(const std::string& sql)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!db) open();

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
        std::string err = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("DBFile exec failed: " + err);
    }
}

void DBFile::select(const std::string& sql,
                    const std::function<void(const std::vector<std::string>&)>& callback)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!db) open();

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::string err = sqlite3_errmsg(db);
        throw std::runtime_error("DBFile select prepare failed: " + err);
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int colCount = sqlite3_column_count(stmt);
        std::vector<std::string> row;
        row.reserve(colCount);

        for (int i = 0; i < colCount; ++i)
        {
            const unsigned char* text = sqlite3_column_text(stmt, i);
            row.emplace_back(text ? reinterpret_cast<const char*>(text) : "");
        }

        callback(row);
    }

    sqlite3_finalize(stmt);
}

void DBFile::selectPrepared(const std::string& sql,
                            const std::vector<std::string>& params,
                            const std::function<void(const std::vector<std::string>&)>& callback)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!db) open();

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::string err = sqlite3_errmsg(db);
        throw std::runtime_error("DBFile selectPrepared prepare failed: " + err);
    }

    bindParams(stmt, params);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int colCount = sqlite3_column_count(stmt);
        std::vector<std::string> row;
        row.reserve(colCount);

        for (int i = 0; i < colCount; ++i)
        {
            const unsigned char* text = sqlite3_column_text(stmt, i);
            row.emplace_back(text ? reinterpret_cast<const char*>(text) : "");
        }

        callback(row);
    }

    sqlite3_finalize(stmt);
}

bool DBFile::executePrepared(const std::string& sql, const std::vector<std::string>& params)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!db) open();

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "[DEBUG] DBFile executePrepared prepare failed: " << sqlite3_errmsg(db)
                  << "\n";
        return false;
    }

    bindParams(stmt, params);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

void DBFile::bindParams(sqlite3_stmt* stmt, const std::vector<std::string>& params)
{
    for (size_t i = 0; i < params.size(); ++i)
    {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
    }
}

bool DBFile::isOpen()
{
    std::lock_guard<std::mutex> lock(mutex);
    return db != nullptr;
}
}  // namespace db