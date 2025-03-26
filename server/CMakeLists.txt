cmake_minimum_required(VERSION 3.15)
project(server)

set(CMAKE_CXX_STANDARD 17)

# Use Boost manually (header-only)
include_directories(/opt/homebrew/include)

# Or if you manually downloaded Boost, use this:
# include_directories(/Users/pushpindersingh/dev/boost_1_84_0)

add_executable(server
        src/main.cpp
        src/websocket_server.cpp
)