#include "websocket_server.h"

namespace beast = boost::beast;

WebSocketServer::WebSocketServer(io_context& context, int port)
    : context(context), acceptor(context, tcp::endpoint(tcp::v4(), port))
{
}

void WebSocketServer::run() {
    std::cout << "WebSocket Server running on port "
              << acceptor.local_endpoint().port() << std::endl;

    // Start accepting connections
    start_accept();

    // Run the I/O context to handle asynchronous events
    context.run();
}

void WebSocketServer::start_accept() {
    // Prepare a new WebSocket stream
    auto ws = std::make_shared<ws_stream>(tcp::socket(context));

    // Asynchronously accept an incoming connection
    acceptor.async_accept(ws->next_layer(), [this, ws](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "Client connected!" << std::endl;
            // Add to our set of clients
            clients.insert(ws);
            // Handle the WebSocket session
            handle_session(ws);
        } else {
            std::cerr << "Accept error: " << ec.message() << std::endl;
        }

        // Continue accepting further connections
        start_accept();
    });
}

void WebSocketServer::handle_session(std::shared_ptr<ws_stream> ws) {
    // Perform the WebSocket handshake
    ws->async_accept([this, ws](boost::system::error_code ec) {
        if (!ec) {
            // Create a buffer for reading messages
            auto buffer = std::make_shared<flat_buffer>();
            // Start reading messages from this client
            handle_read(ws, buffer);
        } else {
            std::cout << "Handshake error: " << ec.message() << std::endl;
            clients.erase(ws);
        }
    });
}

void WebSocketServer::handle_read(std::shared_ptr<ws_stream> ws, std::shared_ptr<flat_buffer> buffer) {
    // Asynchronously read a message from the client
    ws->async_read(*buffer, [this, ws, buffer](boost::system::error_code ec, std::size_t) {
        if (!ec) {
            // Convert buffer to string
            std::string received = beast::buffers_to_string(buffer->data());
            std::cout << "Received message: " << received << std::endl;

            // Echo the message to all connected clients
            for (auto& client : clients) {
                client->async_write(buffer->data(),
                    [](boost::system::error_code /*ec*/, std::size_t /*bytes_transferred*/) {
                        // Error handling/logging can be done here
                    }
                );
            }

            // Clear the buffer for the next message
            buffer->consume(buffer->size());

            // Recursively wait for the next message
            handle_read(ws, buffer);
        } else {
            // On error or disconnect, remove the client
            std::cout << "Read error: " << ec.message() << std::endl;
            clients.erase(ws);
        }
    });
}
