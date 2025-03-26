#include "websocket_server.h"

int main() {
    // Create an I/O context
    boost::asio::io_context context;

    // Instantiate the WebSocket server on port 9000
    WebSocketServer server(context, 9000);

    // Run the server
    server.run();

    return 0;
}
