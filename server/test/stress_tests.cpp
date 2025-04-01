// server/test/stress_tests.cpp
#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <iostream>
#include <cstdio>
#include "DatabaseManager.h"
#include "websocket_server.h"
#include <nlohmann/json.hpp>

using tcp = boost::asio::ip::tcp;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using json = nlohmann::json;

static const int STRESS_TEST_PORT = 9002;
static const int NUM_CLIENTS = 50;  // Number of concurrent clients

// StressTest fixture starts the server in a separate thread using a dedicated database.
class StressTest : public ::testing::Test {
protected:
    boost::asio::io_context serverIo;
    DatabaseManager dbManager{"stress_test.db"};
    WebSocketServer server;
    std::thread serverThread;

    StressTest() 
      : server(serverIo, STRESS_TEST_PORT, dbManager) 
    {
        // Initialize the test database and start the server.
        dbManager.initDB();
        serverThread = std::thread([this]() {
            server.start_accept();
            serverIo.run();
        });
        // Give the server time to start.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~StressTest() override {
        serverIo.stop();
        if (serverThread.joinable())
            serverThread.join();
        std::remove("stress_test.db");
    }
};

TEST_F(StressTest, ConcurrentMessageBroadcast) {
    std::vector<double> latencies;
    std::mutex latenciesMutex;
    std::vector<std::thread> clientThreads;

    // Each client will connect, send a message, and wait for the broadcast (its own message).
    auto clientFunc = [&latencies, &latenciesMutex]() {
        try {
            asio::io_context io;
            tcp::resolver resolver(io);
            auto const results = resolver.resolve("127.0.0.1", std::to_string(STRESS_TEST_PORT));
            websocket::stream<tcp::socket> ws(io);
            asio::connect(ws.next_layer(), results.begin(), results.end());
            ws.handshake("localhost", "/");

            // Create the test message.
            json msg = {
                {"type", "message"},
                {"from", "stress_client"},
                {"room", "stress_room"},
                {"content", "Hello from stress test"},
                {"timestamp", "2025-04-01T00:00:00Z"}
            };

            auto start = std::chrono::steady_clock::now();
            ws.write(asio::buffer(msg.dump()));

            // Wait for the broadcast message (assumes client receives its own message).
            beast::flat_buffer buffer;
            ws.read(buffer);
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            {
                std::lock_guard<std::mutex> lock(latenciesMutex);
                latencies.push_back(elapsed.count());
            }
            ws.close(websocket::close_code::normal);
        } catch (const std::exception &e) {
            std::cerr << "Client exception: " << e.what() << std::endl;
        }
    };

    // Launch NUM_CLIENTS concurrently.
    for (int i = 0; i < NUM_CLIENTS; i++) {
        clientThreads.emplace_back(clientFunc);
    }

    // Wait for all clients to complete.
    for (auto &t : clientThreads) {
        if (t.joinable())
            t.join();
    }

    // Compute total time and average latency.
    double totalLatency = 0.0;
    for (double lat : latencies) {
        totalLatency += lat;
    }
    double avgLatency = totalLatency / latencies.size();
    double throughput = NUM_CLIENTS / totalLatency; // messages per second

    std::cout << "Stress Test: " << NUM_CLIENTS << " clients, total time = " 
              << totalLatency << " seconds" << std::endl;
    std::cout << "Average latency: " << avgLatency << " seconds" << std::endl;
    std::cout << "Throughput: " << throughput << " messages per second" << std::endl;

    // Assert that average latency is under the desired threshold.
    EXPECT_LT(avgLatency, 0.1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
