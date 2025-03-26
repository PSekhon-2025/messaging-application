#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <memory>
#include <set>

using namespace boost::asio;
using namespace boost::beast;
using tcp = ip::tcp;
using ws_stream = websocket::stream<tcp::socket>;

class WebSocketServer {
public:
    WebSocketServer(io_context& context, int port);
    void run();

private:
    void start_accept();
    void handle_session(std::shared_ptr<ws_stream> ws);

    io_context& context;
    tcp::acceptor acceptor;
    std::set<std::shared_ptr<ws_stream>> clients;
};

#endif
