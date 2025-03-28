cmake_minimum_required(VERSION 3.15)
project(server)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
cmake_minimum_required(VERSION 3.15)
project(server)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Include Boost headers
include_directories(${Boost_INCLUDE_DIRS})

# Add your source files, including DatabaseManager for message persistence
add_executable(server
        src/main.cpp
        src/websocket_server.cpp
        src/DatabaseManager.cpp
)

# Link against Boost, SQLite3 and nlohmann_json
target_link_libraries(server PRIVATE
        ${Boost_LIBRARIES}
        SQLite::SQLite3
        nlohmann_json::nlohmann_json
)
