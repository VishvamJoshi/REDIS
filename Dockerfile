# --- Build Stage ---
FROM debian:bookworm AS build

# Install MinGW-w64 (64-bit, POSIX), g++, make, and wine64 for testing
RUN apt-get update && \
    apt-get install -y mingw-w64 g++ make wine64 g++-mingw-w64-x86-64-posix && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy all source files
COPY . .

# Build the server for 64-bit Windows using POSIX threads
RUN x86_64-w64-mingw32-g++-posix --version
RUN x86_64-w64-mingw32-g++-posix -std=c++11 -pthread -o redis_server.exe server.cpp hashtable.cpp heap.cpp thread_pool.cpp zset.cpp avl.cpp -lws2_32

# --- Runtime Stage ---
FROM debian:bullseye
RUN apt-get update && apt-get install -y wine64 && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Copy the built executable
COPY --from=build /app/redis_server.exe ./

# Copy the DLLs from your local project into the image
COPY dlls/libwinpthread-1.dll ./
COPY dlls/libgcc_s_seh-1.dll ./
COPY dlls/libstdc++-6.dll ./

EXPOSE 6379
CMD ["wine64", "redis_server.exe"] 