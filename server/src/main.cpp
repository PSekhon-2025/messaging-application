#include "websocket_server.h"
#include "DatabaseManager.h"
#include <boost/asio.hpp>
#include <iostream>

int main() {
    boost::asio::io_context context;

    // Initialize the database (the file "users.db" will be used)
    DatabaseManager dbManager("local.db");
    if (!dbManager.initDB()) {
        std::cerr << "Database initialization failed." << std::endl;
        return 1;
    }

    // Pass the DatabaseManager instance to the WebSocket server.
    WebSocketServer server(context, 9000, dbManager);
    server.run();
    return 0;
}
