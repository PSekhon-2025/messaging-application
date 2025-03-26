#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <memory>
#include <set>
#include <unordered_map>

// Namespace shortcuts
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

// Type aliases
using ws_stream = websocket::stream<tcp::socket>;
using flat_buffer = beast::flat_buffer;

class WebSocketServer {
public:
    WebSocketServer(asio::io_context& context, int port);
    void run();

private:
    void start_accept();
    void handle_session(std::shared_ptr<ws_stream> ws);
    void handle_read(std::shared_ptr<ws_stream> ws, std::shared_ptr<flat_buffer> buffer);
    void handle_login(const std::string& username, std::shared_ptr<ws_stream> ws);

    asio::io_context& context;
    tcp::acceptor acceptor;
    std::set<std::shared_ptr<ws_stream>> clients;

    std::unordered_map<std::string, std::shared_ptr<ws_stream>> user_sessions;
};

#endif // WEBSOCKET_SERVER_H
