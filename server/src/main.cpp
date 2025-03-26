#include "websocket_server.h"

int main() {
    boost::asio::io_context context;
    WebSocketServer server(context, 9000);  // WebSocket running on port 9000
    server.run();
    return 0;
}