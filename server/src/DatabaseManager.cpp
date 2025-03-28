#include "DatabaseManager.h"
#include <iostream>

DatabaseManager::DatabaseManager(const std::string &dbFile) : db(nullptr), dbFile(dbFile) {}

DatabaseManager::~DatabaseManager() {
    if (db) {
        sqlite3_close(db);
    }
}

bool DatabaseManager::initDB() {
    std::cout << "Database initialized." << std::endl;
    int rc = sqlite3_open(dbFile.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // Create the users table
    const char* createUsersSQL =
        "CREATE TABLE IF NOT EXISTS users ("
        "username TEXT PRIMARY KEY, "
        "password TEXT NOT NULL"
        ");";
    std::cout << createUsersSQL << std::endl;
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createUsersSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error (table creation): " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    // Create the messages table
    const char* createMessagesSQL =
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "room TEXT NOT NULL, "
        "sender TEXT NOT NULL, "
        "content TEXT NOT NULL, "
        "timestamp TEXT NOT NULL"
        ");";
    rc = sqlite3_exec(db, createMessagesSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error (messages table creation): " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DatabaseManager::registerUser(const std::string &username, const std::string &password) {
    // NOTE: In production, hash the password before storing it.
    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to insert user: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    return true;
}

bool DatabaseManager::authenticateUser(const std::string &username, const std::string &password) {
    const char* sql = "SELECT password FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* dbPassword = sqlite3_column_text(stmt, 0);
        bool auth = (password == reinterpret_cast<const char*>(dbPassword));
        sqlite3_finalize(stmt);
        return auth;
    } else {
        sqlite3_finalize(stmt);
        return false;
    }
}

bool DatabaseManager::storeMessage(const std::string &room, const std::string &sender, const std::string &content, const std::string &timestamp) {
    const char* sql = "INSERT INTO messages (room, sender, content, timestamp) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement for storing message: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, room.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, sender.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, timestamp.c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to store message: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    return true;
}

std::vector<nlohmann::json> DatabaseManager::getMessagesForRoom(const std::string &room) {
    std::vector<nlohmann::json> messages;
    const char* sql = "SELECT sender, content, timestamp FROM messages WHERE room = ? ORDER BY id ASC;";
    std::cout << sql << std::endl;
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement for loading messages: " << sqlite3_errmsg(db) << std::endl;
        return messages;
    }

    sqlite3_bind_text(stmt, 1, room.c_str(), -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* senderText = sqlite3_column_text(stmt, 0);
        const unsigned char* contentText = sqlite3_column_text(stmt, 1);
        const unsigned char* timestampText = sqlite3_column_text(stmt, 2);
        nlohmann::json message = {
            {"type", "message"},
            {"room", room}, // add this line
            {"from", reinterpret_cast<const char*>(senderText)},
            {"content", reinterpret_cast<const char*>(contentText)},
            {"timestamp", reinterpret_cast<const char*>(timestampText)}
        };

        messages.push_back(message);
    }
    sqlite3_finalize(stmt);
    return messages;
}
