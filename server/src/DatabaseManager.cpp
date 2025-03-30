#include "DatabaseManager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <chrono>
#include <thread>

// Helper function to compute SHA-256 hash of a password
static std::string hashPassword(const std::string &password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
         ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

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

    // Set a busy timeout of 3000ms so SQLite waits for locks to clear
    sqlite3_busy_timeout(db, 3000);

    char* errMsg = nullptr;
    const int maxRetries = 3;
    int attempt = 0;

    // Helper lambda for executing a query with retry logic
    auto execWithRetry = [&](const char* sql) -> int {
        int result;
        for (int i = 0; i < maxRetries; i++) {
            result = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
            if (result == SQLITE_BUSY) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } else {
                break;
            }
        }
        return result;
    };

    // Begin transaction
    rc = execWithRetry("BEGIN TRANSACTION;");
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error (begin transaction): " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    // Create the users table
    const char* createUsersSQL =
        "CREATE TABLE IF NOT EXISTS users ("
        "username TEXT PRIMARY KEY, "
        "password TEXT NOT NULL"
        ");";

    rc = execWithRetry(createUsersSQL);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error (users table creation): " << errMsg << std::endl;
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
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
    rc = execWithRetry(createMessagesSQL);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error (messages table creation): " << errMsg << std::endl;
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_free(errMsg);
        return false;
    }

    // Commit transaction
    rc = execWithRetry("COMMIT;");
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error (commit transaction): " << errMsg << std::endl;
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool DatabaseManager::authenticateUser(const std::string &username, const std::string &password) {
    // Hash the provided password for comparison
    std::string hashedPassword = hashPassword(password);

    sqlite3_busy_timeout(db, 3000); // Wait up to 3 seconds if the DB is locked

    const int maxRetries = 3;
    const int retryDelayMs = 100;

    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
        if (rc == SQLITE_BUSY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue;
        } else if (rc != SQLITE_OK) {
            std::cerr << "SQL error (begin transaction): " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }

        const char* sql = "SELECT password FROM users WHERE username = ?;";
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);

        bool auth = false;
        if (rc == SQLITE_BUSY) {
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue;
        } else if (rc == SQLITE_ROW) {
            const unsigned char* dbPassword = sqlite3_column_text(stmt, 0);
            // Compare stored hashed password with hash of provided password
            auth = (hashedPassword == reinterpret_cast<const char*>(dbPassword));
        }

        sqlite3_finalize(stmt);

        rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
        if (rc == SQLITE_BUSY) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue;
        } else if (rc != SQLITE_OK) {
            std::cerr << "SQL error (commit transaction): " << errMsg << std::endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            sqlite3_free(errMsg);
            return false;
        }

        return auth;
    }

    std::cerr << "Failed to authenticate user after multiple retries due to database lock." << std::endl;
    return false;
}

bool DatabaseManager::registerUser(const std::string &username, const std::string &password) {
    // Hash the password before storing it
    std::string hashedPassword = hashPassword(password);

    sqlite3_busy_timeout(db, 3000);

    const int maxRetries = 3;
    const int retryDelayMs = 100;

    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        char* errMsg = nullptr;

        int rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
        if (rc == SQLITE_BUSY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue; // retry transaction
        } else if (rc != SQLITE_OK) {
            std::cerr << "SQL error (begin transaction): " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }

        const char* sql = "INSERT INTO users (username, password) VALUES (?, ?);";
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, hashedPassword.c_str(), -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_BUSY) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue; // retry transaction
        } else if (rc != SQLITE_DONE) {
            std::cerr << "Failed to insert user: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }

        rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
        if (rc == SQLITE_BUSY) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue; // retry transaction
        } else if (rc != SQLITE_OK) {
            std::cerr << "SQL error (commit transaction): " << errMsg << std::endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            sqlite3_free(errMsg);
            return false;
        }

        // Successful execution
        return true;
    }

    std::cerr << "Failed to register user after multiple retries due to database lock." << std::endl;
    return false;
}

bool DatabaseManager::storeMessage(const std::string &room, const std::string &sender, const std::string &content, const std::string &timestamp) {
    sqlite3_busy_timeout(db, 3000);

    const int maxRetries = 3;
    const int retryDelayMs = 100;

    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        char* errMsg = nullptr;

        int rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
        if (rc == SQLITE_BUSY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue;
        } else if (rc != SQLITE_OK) {
            std::cerr << "SQL error (begin transaction): " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }

        const char* sql = "INSERT INTO messages (room, sender, content, timestamp) VALUES (?, ?, ?, ?);";
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement for storing message: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }

        sqlite3_bind_text(stmt, 1, room.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, sender.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, timestamp.c_str(), -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc == SQLITE_BUSY) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue;
        } else if (rc != SQLITE_DONE) {
            std::cerr << "Failed to store message: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }

        rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
        if (rc == SQLITE_BUSY) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue;
        } else if (rc != SQLITE_OK) {
            std::cerr << "SQL error (commit transaction): " << errMsg << std::endl;
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            sqlite3_free(errMsg);
            return false;
        }

        return true;
    }

    std::cerr << "Failed to store message after multiple retries due to database lock." << std::endl;
    return false;
}

std::vector<nlohmann::json> DatabaseManager::getMessagesForRoom(const std::string &room) {
    std::vector<nlohmann::json> messages;

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error (begin transaction): " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return messages;
    }

    const char* sql = "SELECT sender, content, timestamp FROM messages WHERE room = ? ORDER BY id ASC;";
    std::cout << sql << std::endl;
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement for loading messages: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return messages;
    }

    sqlite3_bind_text(stmt, 1, room.c_str(), -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* senderText = sqlite3_column_text(stmt, 0);
        const unsigned char* contentText = sqlite3_column_text(stmt, 1);
        const unsigned char* timestampText = sqlite3_column_text(stmt, 2);
        nlohmann::json message = {
            {"type", "message"},
            {"room", room},
            {"from", reinterpret_cast<const char*>(senderText)},
            {"content", reinterpret_cast<const char*>(contentText)},
            {"timestamp", reinterpret_cast<const char*>(timestampText)}
        };

        messages.push_back(message);
    }
    sqlite3_finalize(stmt);

    rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error (commit transaction): " << errMsg << std::endl;
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sqlite3_free(errMsg);
        return messages;
    }
    return messages;
}
