// server/test/functional_tests.cpp
#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <thread>
#include <chrono>
#include "websocket_server.h"
#include "DatabaseManager.h"
#include <nlohmann/json.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

static const int testPort = 9001;

// Helper fixture to run the server in a separate thread.
class WebSocketServerFixture {
public:
    WebSocketServerFixture() : ioContext(), dbManager("functional_test.db"), server(ioContext, testPort, dbManager) {
        dbManager.initDB();
        serverThread = std::thread([this]() {
            server.start_accept();
            ioContext.run();
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ~WebSocketServerFixture() {
        ioContext.stop();
        if (serverThread.joinable())
            serverThread.join();
        std::remove("functional_test.db");
    }
private:
    asio::io_context ioContext;
    DatabaseManager dbManager;
    WebSocketServer server;
    std::thread serverThread;
};

TEST(WebSocketServerTest, SignupAndLogin) {
    WebSocketServerFixture serverFixture;

    // Set up a WebSocket client.
    asio::io_context clientIo;
    tcp::resolver resolver(clientIo);
    auto const results = resolver.resolve("127.0.0.1", std::to_string(testPort));
    websocket::stream<tcp::socket> ws(clientIo);
    asio::connect(ws.next_layer(), results.begin(), results.end());
    ws.handshake("localhost", "/");

    // Signup message.
    json signupMsg = {
        {"type", "signup"},
        {"username", "functionalUser"},
        {"password", "funcPass"}
    };
    ws.write(asio::buffer(signupMsg.dump()));

    // Read signup response.
    beast::flat_buffer buffer;
    ws.read(buffer);
    auto responseText = beast::buffers_to_string(buffer.data());
    auto responseJson = json::parse(responseText);
    EXPECT_EQ(responseJson["status"], "success");

    // Login message.
    json loginMsg = {
        {"type", "login"},
        {"username", "functionalUser"},
        {"password", "funcPass"}
    };
    ws.write(asio::buffer(loginMsg.dump()));

    buffer.consume(buffer.size());
    ws.read(buffer);
    responseText = beast::buffers_to_string(buffer.data());
    responseJson = json::parse(responseText);
    EXPECT_EQ(responseJson["status"], "success");

    ws.close(websocket::close_code::normal);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
