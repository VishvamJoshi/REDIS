# Set the minimum required version of CMake and the project name
cmake_minimum_required(VERSION 3.10)
project(RedisClone CXX)

# Set the C++ standard to C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Server Executable ---

# Find all the server source files
file(GLOB SERVER_SOURCES "src/server/*.cpp")

# Add the server include directory
include_directories(include/server)
include_directories(src)


# Add the server executable target
add_executable(redis_server ${SERVER_SOURCES})

# --- Client Executable ---

# Find the client source file
file(GLOB CLIENT_SOURCES "src/client/*.cpp")

# Add the client include directory
include_directories(include/client)

# Add the client executable target
add_executable(redis_client ${CLIENT_SOURCES})

# --- Link Libraries (For Windows) ---
if(WIN32)
    target_link_libraries(redis_server ws2_32)
    target_link_libraries(redis_client ws2_32)
endif()

# A little message to show where the executables will be
message(STATUS "Server and client executables will be built in the build directory.")