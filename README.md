# Redis-like Server

A simple Redis-like in-memory database server implemented in C++ for Windows.

## Features

- **Key-Value Storage**: Basic string operations (GET, SET, DEL)
- **Sorted Sets**: ZADD, ZREM, ZSCORE, ZQUERY operations
- **TTL Support**: PEXPIRE and PTTL commands
- **Keys Command**: List all keys in the database
- **Multi-threaded**: Thread pool for background operations
- **Event-driven**: Non-blocking I/O with select() for Windows

## Building

### using g++ cmakelists
    mkdir build
    cd build
    cmake .. -G "MinGW Makefiles"
    mingw32-make

    
### Using g++ (GNU C++ Compiler)

```bash
# Compile the server
g++ -std=c++11 -pthread -o redis_server.exe server.cpp hashtable.cpp heap.cpp thread_pool.cpp zset.cpp avl.cpp

# Compile the client
g++ -std=c++11 -o client.exe client.cpp
```

### Using Visual Studio (Windows)

1. Open Developer Command Prompt
2. Navigate to project directory
3. Run:
```cmd
cl /EHsc server.cpp hashtable.cpp heap.cpp thread_pool.cpp zset.cpp avl.cpp /Fe:redis_server.exe
cl /EHsc client.cpp /Fe:client.exe
```

### Prerequisites

- C++11 compatible compiler (g++ or MSVC)
- Windows OS (for select() and Windows-specific APIs)

## Running

1. **Start the server**: `./redis_server.exe` (listens on port 6379)
2. **Run the client**: `./client.exe` (connects to localhost:6379)
3. **Use Redis commands**: SET, GET, DEL, ZADD, ZSCORE, KEYS, etc.
4. **Stop server**: Press `Ctrl+C` in server terminal
5. **Stop client**: Type `quit` or press `Ctrl+C` in client terminal

## Protocol

The server uses a custom binary protocol:

### Request Format
```
+------+-----+------+-----+------+-----+-----+------+
| nstr | len | str1 | len | str2 | ... | len | strn |
+------+-----+------+-----+------+-----+-----+------+
```

- `nstr`: 4-byte number of strings
- `len`: 4-byte length of each string
- `str1, str2, ...`: string data

### Response Format
```
+------+-----+------+
| len  | tag | data |
+------+-----+------+
```

- `len`: 4-byte message length
- `tag`: 1-byte data type tag
- `data`: response data

### Data Types
- `TAG_NIL (0)`: nil value
- `TAG_ERR (1)`: error code + message
- `TAG_STR (2)`: string
- `TAG_INT (3)`: 64-bit integer
- `TAG_DBL (4)`: double
- `TAG_ARR (5)`: array

## Commands

### String Operations
- `SET key value` - Set a key-value pair
- `GET key` - Get value for a key
- `DEL key` - Delete a key

### Sorted Set Operations
- `ZADD zset score name` - Add member to sorted set
- `ZREM zset name` - Remove member from sorted set
- `ZSCORE zset name` - Get score of member
- `ZQUERY zset score name offset limit` - Query sorted set

### TTL Operations
- `PEXPIRE key ttl_ms` - Set TTL in milliseconds
- `PTTL key` - Get remaining TTL in milliseconds

### Utility
- `KEYS` - List all keys in database

## Architecture

- **Event Loop**: Uses select() for I/O multiplexing
- **Connection Management**: Automatic cleanup of idle connections
- **Memory Management**: Thread pool for large data structure cleanup
- **Data Structures**: Custom hashtable, AVL tree, and heap implementations

## Files

- `server.cpp` - Main server implementation
- `client.cpp` - Test client
- `hashtable.h/cpp` - Hash table implementation
- `zset.h/cpp` - Sorted set implementation
- `heap.h/cpp` - Min-heap for TTL management
- `thread_pool.h/cpp` - Thread pool for background tasks
- `common.h` - Common definitions and macros
- `list.h` - Doubly-linked list implementation 