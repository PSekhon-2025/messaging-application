cmake_minimum_required(VERSION 3.10)
project(WebSocketServer)

set(CMAKE_TOOLCHAIN_FILE "~/vcpkg/scripts/buildsystems/vcpkg.cmake")
find_package(Boost REQUIRED COMPONENTS beast thread)

add_executable(server src/main.cpp src/websocket_server.cpp)
target_include_directories(server PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(server PRIVATE Boost::system Boost::thread Boost::beast pthread)
