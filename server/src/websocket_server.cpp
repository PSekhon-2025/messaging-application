#include "websocket_server.h"
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using ws_stream = websocket::stream<tcp::socket>;
using flat_buffer = beast::flat_buffer;
using json = nlohmann::json;
using io_context = asio::io_context; // only declare once

WebSocketServer::WebSocketServer(io_context& context, int port)
    : context(context), acceptor(context, tcp::endpoint(tcp::v4(), port))
{
}

void WebSocketServer::run() {
    std::cout << "WebSocket Server running on port "
              << acceptor.local_endpoint().port() << std::endl;

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
        } else {
            std::cerr << "Accept error: " << ec.message() << std::endl;
        }

        start_accept();
    });
}

void WebSocketServer::handle_session(std::shared_ptr<ws_stream> ws) {
    ws->async_accept([this, ws](boost::system::error_code ec) {
        if (!ec) {
            auto buffer = std::make_shared<flat_buffer>();
            handle_read(ws, buffer);
        } else {
            std::cout << "Handshake error: " << ec.message() << std::endl;
            clients.erase(ws);
        }
    });
}

void WebSocketServer::handle_login(const std::string& username, std::shared_ptr<ws_stream> ws) {
    user_sessions[username] = ws;
    std::cout << "[Login] User '" << username << "' logged in." << std::endl;
    std::cout << "[Login] Total logged-in users: " << user_sessions.size() << std::endl;
}

// ✅ THIS is the correct place to define handle_read
void WebSocketServer::handle_read(std::shared_ptr<ws_stream> ws, std::shared_ptr<flat_buffer> buffer) {
    ws->async_read(*buffer, [this, ws, buffer](boost::system::error_code ec, std::size_t) {
        if (!ec) {
            std::string received = beast::buffers_to_string(buffer->data());
            std::cout << "Received message: " << received << std::endl;

            try {
                auto j = json::parse(received);

                if (j.contains("type")) {
                    std::string msgType = j["type"];

                    if (msgType == "login" && j.contains("username")) {
                        std::string username = j["username"];
                        handle_login(username, ws);
                    }
                    else if (msgType == "message" && j.contains("from") && j.contains("room") &&
                             (j.contains("text") || j.contains("content"))) {
                        std::string from = j["from"];
                        std::string to = j["room"];
                        // Use "text" if available; otherwise "content"
                        std::string text = j.contains("text") ? j["text"].get<std::string>() : j["content"].get<std::string>();

                        if (to == "test") {
                            // Broadcast to all clients in the chat room
                            std::cout << "[Broadcast] Message from '" << from << "' to chat room '" << to
                                      << "': " << text << std::endl;
                            for (auto &client : clients) {
                                // Optionally skip sender if you don't want them to receive their own message:
                                // if (client == ws) continue;
                                client->async_write(boost::asio::buffer(received),
                                    [from, to](boost::system::error_code ec, std::size_t) {
                                        if (!ec) {
                                            std::cout << "[Broadcast] Delivered message from '" << from
                                                      << "' to chat room '" << to << "'" << std::endl;
                                        } else {
                                            std::cerr << "[Broadcast] Failed to deliver message from '" << from
                                                      << "' to chat room '" << to << "'. Error: " << ec.message()
                                                      << std::endl;
                                        }
                                    });
                            }
                        } else {
                            // Direct message routing to a specific user
                            std::cout << "[Routing] Message from '" << from << "' to '" << to << "': " << text << std::endl;
                            if (user_sessions.find(to) != user_sessions.end()) {
                                auto target_ws = user_sessions[to];
                                target_ws->async_write(boost::asio::buffer(received),
                                    [from, to](boost::system::error_code ec, std::size_t) {
                                        if (!ec) {
                                            std::cout << "[Routing] Delivered message from '" << from
                                                      << "' to '" << to << "'" << std::endl;
                                        } else {
                                            std::cerr << "[Routing] Failed to deliver message from '" << from
                                                      << "' to '" << to << "'. Error: " << ec.message()
                                                      << std::endl;
                                        }
                                    });
                            } else {
                                std::cerr << "[Routing] Recipient '" << to << "' not found. Message from '"
                                          << from << "' not delivered." << std::endl;
                            }
                        }
                    }
                    else {
                        std::cout << "[Info] Unknown or improperly formatted message type received." << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "[Error] JSON parse error: " << e.what() << std::endl;
            }

            buffer->consume(buffer->size());
            handle_read(ws, buffer); // Continue reading for this client
        } else {
            std::cout << "[Disconnect] Client disconnected. Reason: " << ec.message() << std::endl;
            // Cleanup: Remove any users associated with this socket.
            for (auto it = user_sessions.begin(); it != user_sessions.end(); ) {
                if (it->second == ws) {
                    std::cout << "[Cleanup] Removing user: " << it->first << std::endl;
                    it = user_sessions.erase(it);
                } else {
                    ++it;
                }
            }
            clients.erase(ws);
        }
    });
}

