# C++ In-Memory Database (Redis-Like)

A high-performance, Redis-like in-memory database server built from scratch in C++. It features an event-driven, non-blocking architecture and supports multiple data structures and commands, including key-value storage, sorted sets, and a pub/sub messaging system.

## Key Features

* **Core Data Types**: `GET`, `SET`, `DEL`, `APPEND`, `DECR` for string manipulation.
* **Sorted Sets**: `ZADD`, `ZREM`, `ZSCORE`, `ZQUERY` using a composite hash table and AVL tree structure for efficient lookups and ordered queries.
* **Pub/Sub**: `SUBSCRIBE` and `PUBLISH` for real-time messaging patterns between clients.
* **Key Management**: Time-to-live (TTL) support with `PEXPIRE` and `PTTL`, and pattern matching with `KEYS`.
* **High Concurrency**: A non-blocking I/O model using `select()` on Windows to handle thousands of concurrent clients efficiently.
* **Efficient Background Processing**: A thread pool is used to offload expensive cleanup of large data structures, preventing the main event loop from blocking.

## Performance

The server is optimized for high throughput and low latency. Benchmarks were conducted on a local machine in a single-threaded context, demonstrating consistent and reliable performance.

| Metric              | SET Command         | GET Command         |
| ------------------- | ------------------- | ------------------- |
| **Throughput** | **~2,500 ops/sec** | **~8,100 ops/sec** |
| **Average Latency** | -                   | **~0.123 ms** |
| **p99 Latency** | -                   | **~0.224 ms** |

These results highlight the server's highly efficient read performance and its ability to maintain consistent, sub-millisecond response times under load.

## Building and Running

The recommended way to build the project is with CMake, which supports various compilers and environments.

### Prerequisites

* C++14 compatible compiler (e.g., MinGW-w64 on Windows, g++ on Linux)
* CMake (version 3.10+)

### Build Steps

```bash
# 1. Clone the repository
git clone <repo-url>
cd <repo-directory>

# 2. Create and navigate to the build directory
mkdir build
cd build

# 3. Generate the build files using CMake
# On Windows with MinGW
cmake .. -G "MinGW Makefiles"

# On Linux
cmake ..

# 4. Compile the project
# On Windows with MinGW
mingw32-make

# On Linux
make




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