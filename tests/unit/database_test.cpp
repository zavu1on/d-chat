#include <gtest/gtest.h>

#include "DBFile.hpp"
#include "test_helpers.hpp"

class DatabaseTest : public ::testing::Test
{
protected:
    std::unique_ptr<test_helpers::TestEnvironment> env;
    std::shared_ptr<db::DBFile> db;

    void SetUp() override
    {
        env = std::make_unique<test_helpers::TestEnvironment>();
        std::string dbPath = env->createTestDatabase("test");
        db = std::make_shared<db::DBFile>(dbPath);
    }

    void TearDown() override
    {
        if (db)
        {
            db->close();
        }
        env->cleanup();
    }
};

TEST_F(DatabaseTest, OpenAndCloseSucceeds)
{
    EXPECT_NO_THROW(db->open());
    EXPECT_TRUE(db->isOpen());

    EXPECT_NO_THROW(db->close());
    EXPECT_FALSE(db->isOpen());
}

TEST_F(DatabaseTest, CreateTableSucceeds)
{
    db->open();

    EXPECT_NO_THROW({
        db->exec(R"(
            CREATE TABLE test_table (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                value INTEGER
            );
        )");
    });
}

TEST_F(DatabaseTest, InsertAndSelectSucceeds)
{
    db->open();

    db->exec(R"(
        CREATE TABLE users (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL
        );
    )");

    EXPECT_TRUE(db->executePrepared("INSERT INTO users(id, name) VALUES (?,?);", { "1", "Alice" }));

    EXPECT_TRUE(db->executePrepared("INSERT INTO users(id, name) VALUES (?,?);", { "2", "Bob" }));

    std::vector<std::string> names;
    db->select("SELECT name FROM users ORDER BY id;",
               [&names](const std::vector<std::string>& row)
               {
                   if (!row.empty()) names.push_back(row[0]);
               });

    ASSERT_EQ(names.size(), 2);
    EXPECT_EQ(names[0], "Alice");
    EXPECT_EQ(names[1], "Bob");
}

TEST_F(DatabaseTest, PreparedStatementHandlesSpecialCharacters)
{
    db->open();

    db->exec(R"(
        CREATE TABLE messages (
            id INTEGER PRIMARY KEY,
            content TEXT NOT NULL
        );
    )");

    std::string specialContent = "Hello 'world' with \"quotes\" and ; semicolons";

    EXPECT_TRUE(db->executePrepared("INSERT INTO messages(id, content) VALUES (?,?);",
                                    { "1", specialContent }));

    std::string retrieved;
    db->select("SELECT content FROM messages WHERE id=1;",
               [&retrieved](const std::vector<std::string>& row)
               {
                   if (!row.empty()) retrieved = row[0];
               });

    EXPECT_EQ(retrieved, specialContent);
}

TEST_F(DatabaseTest, SelectWithNoResultsCallsCallbackZeroTimes)
{
    db->open();

    db->exec(R"(
        CREATE TABLE empty_table (
            id INTEGER PRIMARY KEY
        );
    )");

    int callCount = 0;
    db->select("SELECT * FROM empty_table;",
               [&callCount](const std::vector<std::string>&) { callCount++; });

    EXPECT_EQ(callCount, 0);
}

TEST_F(DatabaseTest, TransactionRollbackWorks)
{
    db->open();

    db->exec(R"(
        CREATE TABLE transactions_test (
            id INTEGER PRIMARY KEY,
            value INTEGER
        );
    )");

    db->exec("BEGIN TRANSACTION;");
    db->executePrepared("INSERT INTO transactions_test(id, value) VALUES (?,?);", { "1", "100" });
    db->exec("ROLLBACK;");

    int count = 0;
    db->select("SELECT COUNT(*) FROM transactions_test;",
               [&count](const std::vector<std::string>& row)
               {
                   if (!row.empty()) count = std::stoi(row[0]);
               });

    EXPECT_EQ(count, 0);
}

TEST_F(DatabaseTest, WALModeIsEnabled)
{
    db->open();

    std::string journalMode;
    db->select("PRAGMA journal_mode;",
               [&journalMode](const std::vector<std::string>& row)
               {
                   if (!row.empty()) journalMode = row[0];
               });

    EXPECT_EQ(journalMode, "wal");
}