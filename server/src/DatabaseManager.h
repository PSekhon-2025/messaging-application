#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

class DatabaseManager {
public:
    DatabaseManager(const std::string &dbFile);
    ~DatabaseManager();

    // Initialize the database (create tables if they don’t exist)
    bool initDB();

    // User management functions
    bool registerUser(const std::string &username, const std::string &password);
    bool authenticateUser(const std::string &username, const std::string &password);

    // Message persistence functions
    bool storeMessage(const std::string &room, const std::string &sender, const std::string &content, const std::string &timestamp);
    std::vector<nlohmann::json> getMessagesForRoom(const std::string &room);

private:
    sqlite3* db;
    std::string dbFile;
};

#endif // DATABASE_MANAGER_H
