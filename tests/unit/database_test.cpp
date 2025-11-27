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

    int countRows(const std::string& tableName)
    {
        int count = 0;
        db->select("SELECT COUNT(*) FROM " + tableName + ";",
                   [&count](const std::vector<std::string>& row)
                   {
                       if (!row.empty()) count = std::stoi(row[0]);
                   });
        return count;
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

class SQLInjectionTest : public DatabaseTest
{
protected:
    void SetUp() override
    {
        DatabaseTest::SetUp();
        db->open();

        db->exec(R"(
            CREATE TABLE users (
                id INTEGER PRIMARY KEY,
                username TEXT NOT NULL,
                password TEXT NOT NULL,
                role TEXT DEFAULT 'user'
            );
        )");

        db->executePrepared("INSERT INTO users(id, username, password, role) VALUES (?,?,?,?);",
                            { "1", "admin", "secret123", "admin" });
        db->executePrepared("INSERT INTO users(id, username, password, role) VALUES (?,?,?,?);",
                            { "2", "alice", "password1", "user" });
        db->executePrepared("INSERT INTO users(id, username, password, role) VALUES (?,?,?,?);",
                            { "3", "bob", "password2", "user" });
    }

    int countUsers() { return countRows("users"); }
};

TEST_F(SQLInjectionTest, PreparedStatementBlocksInjectionInInsert)
{
    std::string maliciousUsername = "hacker'; DROP TABLE users; --";
    std::string maliciousPassword = "'; DELETE FROM users WHERE '1'='1";

    EXPECT_TRUE(db->executePrepared("INSERT INTO users(id, username, password) VALUES (?,?,?);",
                                    { "100", maliciousUsername, maliciousPassword }));

    EXPECT_EQ(countUsers(), 4);

    std::string storedUsername;
    db->select("SELECT username FROM users WHERE id=100;",
               [&storedUsername](const std::vector<std::string>& row)
               {
                   if (!row.empty()) storedUsername = row[0];
               });

    EXPECT_EQ(storedUsername, maliciousUsername);
}

TEST_F(SQLInjectionTest, PreparedStatementBlocksInjectionInDelete)
{
    std::string deleteInjection = "1' OR '1'='1";

    db->executePrepared("DELETE FROM users WHERE id=?;", { deleteInjection });

    EXPECT_EQ(countUsers(), 3);
}

TEST_F(SQLInjectionTest, DangerousInputsStoredSafely)
{
    std::vector<std::string> dangerousInputs = {
        "Robert'); DROP TABLE users;--",        "' OR 1=1--",
        "'; DELETE FROM users WHERE ''='",      "1; UPDATE users SET role='admin'--",
        "' UNION SELECT password FROM users--", "admin'--",
    };

    int id = 1000;
    for (const auto& input : dangerousInputs)
    {
        EXPECT_TRUE(db->executePrepared("INSERT INTO users(id, username, password) VALUES (?,?,?);",
                                        { std::to_string(id), input, "safe_password" }));

        std::string retrieved;
        db->select("SELECT username FROM users WHERE id=" + std::to_string(id) + ";",
                   [&retrieved](const std::vector<std::string>& row)
                   {
                       if (!row.empty()) retrieved = row[0];
                   });

        EXPECT_EQ(retrieved, input) << "Failed for input: " << input;
        ++id;
    }

    EXPECT_EQ(countUsers(), 3 + static_cast<int>(dangerousInputs.size()));
}

TEST_F(SQLInjectionTest, MultiStatementInjectionBlocked)
{
    std::string multiStatement =
        "1; INSERT INTO users(id, username, password) VALUES(999,'hacked','hacked')";

    db->executePrepared("DELETE FROM users WHERE id=?;", { multiStatement });

    std::string hackedUser;
    db->select("SELECT username FROM users WHERE id=999;",
               [&hackedUser](const std::vector<std::string>& row)
               {
                   if (!row.empty()) hackedUser = row[0];
               });

    EXPECT_TRUE(hackedUser.empty()) << "Multi-statement injection should be blocked";
    EXPECT_EQ(countUsers(), 3);
}

TEST_F(SQLInjectionTest, StringConcatenationIsVulnerable)
{
    db->exec(R"(
        CREATE TABLE messages (
            id TEXT PRIMARY KEY,
            content TEXT
        );
    )");
    db->executePrepared("INSERT INTO messages(id, content) VALUES (?,?);",
                        { "msg-001", "Hello World" });
    db->executePrepared("INSERT INTO messages(id, content) VALUES (?,?);",
                        { "msg-002", "Secret Data" });

    std::string maliciousId = "' OR '1'='1";

    int vulnerableResultCount = 0;
    db->select("SELECT content FROM messages WHERE id='" + maliciousId + "';",
               [&vulnerableResultCount](const std::vector<std::string>&)
               { vulnerableResultCount++; });

    EXPECT_EQ(vulnerableResultCount, 2)
        << "WARNING: String concatenation in select() is vulnerable to SQL injection!";
}