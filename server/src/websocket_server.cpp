#include "websocket_server.h"

WebSocketServer::WebSocketServer(io_context& context, int port)
    : context(context), acceptor(context, tcp::endpoint(tcp::v4(), port)) {}

void WebSocketServer::run() {
    std::cout << "WebSocket Server running on port " << acceptor.local_endpoint().port() << std::endl;
    start_accept();
    context.run();
}

void WebSocketServer::start_accept() {
    auto ws = std::make_shared<ws_stream>(tcp::socket(context));
    
    acceptor.async_accept(ws->next_layer(), [this, ws](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "Client connected!" << std::endl;
            clients.insert(ws);
            handle_session(ws);
        }
        start_accept();  // Keep accepting new clients
    });
}

void WebSocketServer::handle_session(std::shared_ptr<ws_stream> ws) {
    ws->async_accept([this, ws](boost::system::error_code ec) {
        if (!ec) {
            auto buffer = std::make_shared<flat_buffer>();
            ws->async_read(*buffer, [this, ws, buffer](boost::system::error_code ec, std::size_t) {
                if (!ec) {
                    std::string received = buffers_to_string(buffer->data());
                    std::cout << "Received message: " << received << std::endl;
                    
                    // Echo back to all clients
                    for (auto& client : clients) {
                        client->async_write(buffer->data(), [](boost::system::error_code, std::size_t) {});
                    }
                }
            });
        }
    });
}
