// server/test/unit_tests.cpp
#include <gtest/gtest.h>
#include "DatabaseManager.h"
#include <cstdio> // For remove()

// Fixture for tests using a temporary test database.
class DatabaseManagerTest : public ::testing::Test {
protected:
    std::string testDB = "unit_test.db";
    void SetUp() override {
        std::remove(testDB.c_str());
    }
    void TearDown() override {
        std::remove(testDB.c_str());
    }
};

TEST_F(DatabaseManagerTest, InitDB) {
    DatabaseManager dbManager(testDB);
    EXPECT_TRUE(dbManager.initDB());
}

TEST_F(DatabaseManagerTest, UserRegistrationAndAuthentication) {
    DatabaseManager dbManager(testDB);
    ASSERT_TRUE(dbManager.initDB());

    // Register a new user.
    EXPECT_TRUE(dbManager.registerUser("testuser", "password123"));

    // Duplicate registration should fail.
    EXPECT_FALSE(dbManager.registerUser("testuser", "password123"));

    // Authenticate with correct credentials.
    EXPECT_TRUE(dbManager.authenticateUser("testuser", "password123"));

    // Authenticate with an incorrect password.
    EXPECT_FALSE(dbManager.authenticateUser("testuser", "wrongpassword"));
}

TEST_F(DatabaseManagerTest, MessageStorageAndRetrieval) {
    DatabaseManager dbManager(testDB);
    ASSERT_TRUE(dbManager.initDB());

    std::string room = "general";
    std::string sender = "testuser";
    std::string content = "Hello, World!";
    std::string timestamp = "2025-03-31T17:00:00Z";

    // Store a message.
    EXPECT_TRUE(dbManager.storeMessage(room, sender, content, timestamp));

    // Retrieve messages for the room.
    auto messages = dbManager.getMessagesForRoom(room);
    ASSERT_FALSE(messages.empty());
    EXPECT_EQ(messages[0]["from"], sender);
    EXPECT_EQ(messages[0]["content"], content);
    EXPECT_EQ(messages[0]["timestamp"], timestamp);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
