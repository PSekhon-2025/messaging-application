// websocket_server.h

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <set>
#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;
using io_context = boost::asio::io_context;
using ws_stream = websocket::stream<tcp::socket>;
using flat_buffer = beast::flat_buffer;

class WebSocketServer {
public:
    WebSocketServer(io_context& context, int port);
    void run();

private:
    void start_accept();
    void handle_session(std::shared_ptr<ws_stream> ws);

    // 🔧 This declaration is REQUIRED
    void handle_read(std::shared_ptr<ws_stream> ws, std::shared_ptr<flat_buffer> buffer);

    io_context& context;
    tcp::acceptor acceptor;
    std::set<std::shared_ptr<ws_stream>> clients;
};

#endif // WEBSOCKET_SERVER_H
