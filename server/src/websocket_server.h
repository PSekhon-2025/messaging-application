#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <set>
#include <unordered_map>
#include <memory>
#include <deque>
#include <string>
#include "DatabaseManager.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

// Forward declaration of Session.
struct Session;

// WebSocketServer now uses Session objects.
class WebSocketServer {
public:
    WebSocketServer(asio::io_context& context, int port, DatabaseManager &dbManager);
    // Add start_accept() here so it's accessible from main.cpp
    void start_accept();

    // You may also leave run() if it's still needed.
    void run();

private:
    asio::io_context &context;
    tcp::acceptor acceptor;
    // Active sessions for all connected clients.
    std::set<std::shared_ptr<Session>> sessions;
    // Map of username to session for logged-in users.
    std::unordered_map<std::string, std::shared_ptr<Session>> user_sessions;
    DatabaseManager &dbManager;


    void handle_session(std::shared_ptr<Session> session);
    void handle_read(std::shared_ptr<Session> session, std::shared_ptr<beast::flat_buffer> buffer);
    void handle_login(const std::string &username, std::shared_ptr<Session> session);
};

// Session wraps a websocket stream and serializes write operations.
struct Session : public std::enable_shared_from_this<Session> {
    std::shared_ptr<websocket::stream<tcp::socket>> ws;
    // Use the io_context's executor for the strand.
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    // Queue of messages to send.
    std::deque<std::string> write_queue;

    // Constructor now takes the io_context reference.
    Session(std::shared_ptr<websocket::stream<tcp::socket>> ws, asio::io_context &context)
        : ws(ws), strand(context.get_executor()) {}

    // Enqueue a message and initiate writing if necessary.
    void write(const std::string &msg);

    // Helper to perform an async_write for the front message in the queue.
    void do_write();
};

#endif // WEBSOCKET_SERVER_H
