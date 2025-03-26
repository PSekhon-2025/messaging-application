cmake_minimum_required(VERSION 3.15)
project(server)

set(CMAKE_CXX_STANDARD 17)

# Let CMake find Boost and include it properly
find_package(Boost REQUIRED COMPONENTS system thread)

# Include Boost headers
include_directories(${Boost_INCLUDE_DIRS})

# Your source files
add_executable(server
        src/main.cpp
        src/websocket_server.cpp
)

# Link Boost libraries and pthread
target_link_libraries(server
        PRIVATE
        ${Boost_LIBRARIES}
        pthread
)
