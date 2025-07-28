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
git clone <your-repo-url>
cd <your-repo-directory>

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