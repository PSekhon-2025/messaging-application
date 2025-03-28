#include "websocket_server.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <functional>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using flat_buffer = beast::flat_buffer;
using json = nlohmann::json;

//----------------------
// Session member functions
//----------------------
void Session::write(const std::string &msg) {
    auto self = shared_from_this();
    boost::asio::post(strand, [this, self, msg]() {
        bool write_in_progress = !write_queue.empty();
        write_queue.push_back(msg);
        if (!write_in_progress) {
            do_write();
        }
    });
}

void Session::do_write() {
    auto self = shared_from_this();
    ws->async_write(boost::asio::buffer(write_queue.front()),
        boost::asio::bind_executor(strand,
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_queue.pop_front();
                    if (!write_queue.empty()) {
                        do_write();
                    }
                } else {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }
            }));
}


//----------------------
// WebSocketServer member functions
//----------------------
WebSocketServer::WebSocketServer(asio::io_context& context, int port, DatabaseManager &dbManager)
    : context(context), acceptor(context, tcp::endpoint(tcp::v4(), port)), dbManager(dbManager)
{
}

void WebSocketServer::run() {
    std::cout << "WebSocket Server running on port " << acceptor.local_endpoint().port() << std::endl;
    start_accept();
    context.run();
}

void WebSocketServer::start_accept() {
    auto ws = std::make_shared<websocket::stream<tcp::socket>>(tcp::socket(context));
    auto session = std::make_shared<Session>(ws, context);
    acceptor.async_accept(ws->next_layer(), [this, session](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "Client connected!" << std::endl;
            sessions.insert(session);
            handle_session(session);
        } else {
            std::cerr << "Accept error: " << ec.message() << std::endl;
        }
        start_accept();
    });
}


void WebSocketServer::handle_session(std::shared_ptr<Session> session) {
    session->ws->async_accept([this, session](boost::system::error_code ec) {
        if (!ec) {
            auto buffer = std::make_shared<flat_buffer>();
            handle_read(session, buffer);
        } else {
            std::cout << "Handshake error: " << ec.message() << std::endl;
            sessions.erase(session);
        }
    });
}

void WebSocketServer::handle_login(const std::string& username, std::shared_ptr<Session> session) {
    user_sessions[username] = session;
    std::cout << "[Login] User '" << username << "' logged in." << std::endl;
    std::cout << "[Login] Total logged-in users: " << user_sessions.size() << std::endl;
}

void WebSocketServer::handle_read(std::shared_ptr<Session> session, std::shared_ptr<flat_buffer> buffer) {
    session->ws->async_read(*buffer, [this, session, buffer](boost::system::error_code ec, std::size_t) {
        if (!ec) {
            std::string received = beast::buffers_to_string(buffer->data());
            std::cout << "Received message: " << received << std::endl;
            try {
                auto j = json::parse(received);
                if (j.contains("type")) {
                    std::string msgType = j["type"];

                    // LOGIN handling: verify credentials against the database
                    if (msgType == "login" && j.contains("username") && j.contains("password")) {
                        std::string username = j["username"];
                        std::string password = j["password"];
                        if (dbManager.authenticateUser(username, password)) {
                            handle_login(username, session);
                            json response = {
                                {"type", "login_response"},
                                {"status", "success"},
                                {"message", "Login successful"}
                            };
                            session->write(response.dump());
                        } else {
                            json response = {
                                {"type", "login_response"},
                                {"status", "error"},
                                {"message", "Invalid credentials"}
                            };
                            session->write(response.dump());
                        }
                    }
                    // JOIN handling: associate user with room and send history sequentially
                    else if (msgType == "join" && j.contains("username") && j.contains("room")) {
                        std::string username = j["username"];
                        std::string room = j["room"];
                        handle_login(username, session);
                        std::cout << "[Join] User '" << username << "' joined room '" << room << "'" << std::endl;
                        json response = {
                            {"type", "join_response"},
                            {"status", "success"},
                            {"message", "Joined room successfully"}
                        };
                        session->write(response.dump());
                        // Load previous messages for the room and send them sequentially.
                        auto history = dbManager.getMessagesForRoom(room);
                        std::function<void(std::size_t)> send_history;
                        send_history = [session, history, &send_history](std::size_t index) mutable {
                            if (index < history.size()) {
                                session->write(history[index].dump());
                                send_history(index + 1);
                            }
                        };
                        send_history(0);
                    }
                    // SIGNUP handling: register the user in the database
                    else if (msgType == "signup" && j.contains("username") && j.contains("password")) {
                        std::string username = j["username"];
                        std::string password = j["password"];
                        if (dbManager.registerUser(username, password)) {
                            json response = {
                                {"type", "signup_response"},
                                {"status", "success"},
                                {"message", "Registration successful"}
                            };
                            session->write(response.dump());
                        } else {
                            json response = {
                                {"type", "signup_response"},
                                {"status", "error"},
                                {"message", "Registration failed, username may already exist"}
                            };
                            session->write(response.dump());
                        }
                    }
                    // Message handling: broadcast or route messages and store them in the database
                    else if (msgType == "message" && j.contains("from") && j.contains("room") &&
                             (j.contains("text") || j.contains("content"))) {
                        std::string from = j["from"];
                        std::string room = j["room"];
                        std::string text = j.contains("text") ? j["text"].get<std::string>() : j["content"].get<std::string>();
                        std::string timestamp = j.contains("timestamp") ? j["timestamp"].get<std::string>() : "";

                        // Store the message in the database.
                        if (!dbManager.storeMessage(room, from, text, timestamp)) {
                            std::cerr << "[DB] Failed to store message from '" << from << "' in room '" << room << "'" << std::endl;
                        }

                        if (room == "test") {
                            std::cout << "[Broadcast] Message from '" << from << "' to chat room '" << room << "': " << text << std::endl;
                            // Broadcast the message to all sessions.
                            for (auto &s : sessions) {
                                s->write(received);
                            }
                        } else {
                            std::cout << "[Routing] Message from '" << from << "' to '" << room << "': " << text << std::endl;
                            if (user_sessions.find(room) != user_sessions.end()) {
                                auto target_session = user_sessions[room];
                                target_session->write(received);
                            } else {
                                std::cerr << "[Routing] Recipient '" << room << "' not found. Message from '"
                                          << from << "' not delivered." << std::endl;
                            }
                        }
                    } else {
                        std::cout << "[Info] Unknown or improperly formatted message type received." << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "[Error] JSON parse error: " << e.what() << std::endl;
            }
            buffer->consume(buffer->size());
            handle_read(session, buffer); // Continue reading messages
        } else {
            std::cout << "[Disconnect] Client disconnected. Reason: " << ec.message() << std::endl;
            for (auto it = user_sessions.begin(); it != user_sessions.end(); ) {
                if (it->second == session) {
                    std::cout << "[Cleanup] Removing user: " << it->first << std::endl;
                    it = user_sessions.erase(it);
                } else {
                    ++it;
                }
            }
            sessions.erase(session);
        }
    });
}
