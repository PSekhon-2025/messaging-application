#include "websocket_server.h"
#include "DatabaseManager.h"
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    // Create an io_context object
    boost::asio::io_context ioContext;

    // Create a work guard to prevent ioContext from stopping when there are no immediate tasks.
    auto workGuard = boost::asio::make_work_guard(ioContext);

    // Initialize the database.
    DatabaseManager dbManager("local.db");
    if (!dbManager.initDB()) {
        std::cerr << "Database initialization failed." << std::endl;
        return 1;
    }

    // Create the WebSocket server instance.
    // Note: Use a method that only starts accepting connections (instead of calling ioContext.run() inside)
    WebSocketServer server(ioContext, 9000, dbManager);
    server.start_accept();  // Start accepting connections

    // Determine the number of threads to use in the thread pool.
    unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) { // Fallback if hardware_concurrency() can't determine the number of cores.
        threadCount = 2;
    }

    std::cout << "Starting io_context on " << threadCount << " threads." << std::endl;

    // Create and launch the thread pool.
    std::vector<std::thread> threadPool;
    for (unsigned int i = 0; i < threadCount; ++i) {
        threadPool.emplace_back([&ioContext]() {
            ioContext.run();
        });
    }

    // Wait for all threads to finish.
    for (auto& thread : threadPool) {
        thread.join();
    }

    return 0;
}
