#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include "timer.h" 
#include <numeric>
#include <algorithm>
// Protocol tags
constexpr uint8_t TAG_NIL = 0;
constexpr uint8_t TAG_ERR = 1;
constexpr uint8_t TAG_STR = 2;
constexpr uint8_t TAG_INT = 3;
constexpr uint8_t TAG_DBL = 4;
constexpr uint8_t TAG_ARR = 5;



void append_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.insert(buf.end(), (uint8_t*)&val, (uint8_t*)&val + 4);
}

void append_str(std::vector<uint8_t>& buf, const std::string& str) {
    append_u32(buf, (uint32_t)str.size());
    buf.insert(buf.end(), str.begin(), str.end());
}

bool send_command_quiet(SOCKET sock, const std::vector<std::string>& cmd) {
    if (cmd.empty()) return true;

    // Build request
    std::vector<uint8_t> req_buf;
    uint32_t n_args = cmd.size();
    req_buf.insert(req_buf.end(), (uint8_t*)&n_args, (uint8_t*)&n_args + 4);
    for (const auto& arg : cmd) {
        uint32_t len = (uint32_t)arg.size();
        req_buf.insert(req_buf.end(), (uint8_t*)&len, (uint8_t*)&len + 4);
        req_buf.insert(req_buf.end(), arg.begin(), arg.end());
    }

    // Prepend total length
    uint32_t total_len = (uint32_t)req_buf.size();
    std::vector<uint8_t> full_request;
    full_request.insert(full_request.end(), (uint8_t*)&total_len, (uint8_t*)&total_len + 4);
    full_request.insert(full_request.end(), req_buf.begin(), req_buf.end());
    
    // Send request
    if (send(sock, (char*)full_request.data(), (int)full_request.size(), 0) == SOCKET_ERROR) {
        return false;
    }

    // Read response header
    uint8_t header[4];
    int recvd = 0;
    while (recvd < 4) {
        int r = recv(sock, (char*)header + recvd, 4 - recvd, 0);
        if (r <= 0) return false;
        recvd += r;
    }

    // Read response body
    uint32_t resp_len;
    memcpy(&resp_len, header, 4);
    if (resp_len > 0) {
        std::vector<uint8_t> response(resp_len);
        recvd = 0;
        while (recvd < (int)resp_len) {
            int r = recv(sock, (char*)response.data() + recvd, resp_len - recvd, 0);
            if (r <= 0) return false;
            recvd += r;
        }
    }
    return true;
}

void run_benchmark(SOCKET fd) {
    const int NUM_OPS = 50000; // Use 50k for a quicker, yet meaningful test
    std::vector<std::vector<std::string>> set_cmds;
    std::vector<std::vector<std::string>> get_cmds;

    std::cout << "Preparing " << NUM_OPS << " commands for benchmarking..." << std::endl;

    for (int i = 0; i < NUM_OPS; ++i) {
        set_cmds.push_back({"set", "key" + std::to_string(i), "value" + std::to_string(i)});
        get_cmds.push_back({"get", "key" + std::to_string(i)});
    }

    std::cout << "--- BENCHMARKING ---" << std::endl;

    // 1. Benchmark SET operations (Throughput)
    uint64_t start_time = get_time_usec();
    for (const auto& cmd : set_cmds) {
        send_command_quiet(fd, cmd);
    }
    uint64_t end_time = get_time_usec();

    double set_duration_sec = (end_time - start_time) / 1000000.0;
    double set_ops = NUM_OPS / set_duration_sec;
    std::cout << "[SET] " << NUM_OPS << " requests in " << std::fixed << std::setprecision(2) << set_duration_sec << " seconds" << std::endl;
    std::cout << "      Throughput: " << std::fixed << std::setprecision(0) << set_ops << " ops/sec" << std::endl << std::endl;

    // 2. Benchmark GET operations (Throughput & Latency)
    std::vector<double> latencies;
    latencies.reserve(NUM_OPS);
    
    start_time = get_time_usec();
    for (const auto& cmd : get_cmds) {
        uint64_t op_start_time = get_time_usec();
        send_command_quiet(fd, cmd);
        uint64_t op_end_time = get_time_usec();
        latencies.push_back((op_end_time - op_start_time) / 1000.0); // Store latency in milliseconds
    }
    end_time = get_time_usec();

    double get_duration_sec = (end_time - start_time) / 1000000.0;
    double get_ops = NUM_OPS / get_duration_sec;
    std::cout << "[GET] " << NUM_OPS << " requests in " << std::fixed << std::setprecision(2) << get_duration_sec << " seconds" << std::endl;
    std::cout << "      Throughput: " << std::fixed << std::setprecision(0) << get_ops << " ops/sec" << std::endl;

    // Calculate latency stats
    std::sort(latencies.begin(), latencies.end());
    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double avg_latency_ms = sum / NUM_OPS;
    double p99_latency_ms = latencies[static_cast<size_t>(NUM_OPS * 0.99)];

    std::cout << "      Avg Latency: " << std::fixed << std::setprecision(3) << avg_latency_ms << " ms" << std::endl;
    std::cout << "      p99 Latency: " << std::fixed << std::setprecision(3) << p99_latency_ms << " ms" << std::endl;
}



std::vector<uint8_t> make_request(const std::vector<std::string>& args) {
    std::vector<uint8_t> buf;
    uint32_t n_args = args.size();
    buf.insert(buf.end(), (uint8_t*)&n_args, (uint8_t*)&n_args + 4);
    for (const auto& arg : args) {
        uint32_t len = (uint32_t)arg.size();
        buf.insert(buf.end(), (uint8_t*)&len, (uint8_t*)&len + 4);
        buf.insert(buf.end(), arg.begin(), arg.end());
    }
    return buf;
}


// Helper to read a value from a buffer
uint32_t read_u32(const uint8_t*& p) {
    uint32_t v;
    memcpy(&v, p, 4);
    p += 4;
    return v;
}
int64_t read_i64(const uint8_t*& p) {
    int64_t v;
    memcpy(&v, p, 8);
    p += 8;
    return v;
}
double read_dbl(const uint8_t*& p) {
    double v;
    memcpy(&v, p, 8);
    p += 8;
    return v;
}
std::string read_str(const uint8_t*& p) {
    uint32_t len = read_u32(p);
    std::string s((const char*)p, len);
    p += len;
    return s;
}


// Recursive protocol parser
void print_value(const uint8_t*& p, int indent = 0) {
    auto pad = [indent]() { for (int i = 0; i < indent; ++i) std::cout << "  "; };
    uint8_t tag = *p++;
    switch (tag) {
        case TAG_NIL:
            pad(); std::cout << "(nil)" << std::endl;
            break;
        case TAG_ERR: {
            uint32_t code = read_u32(p);
            std::string msg = read_str(p);
            pad(); std::cout << "[ERROR " << code << "]: " << msg << std::endl;
            break;
        }
        case TAG_STR: {
            std::string s = read_str(p);
            pad(); std::cout << s << std::endl;
            break;
        }
        case TAG_INT: {
            int64_t v = read_i64(p);
            pad(); std::cout << v << std::endl;
            break;
        }
        case TAG_DBL: {
            double d = read_dbl(p);
            pad(); std::cout << d << std::endl;
            break;
        }
        case TAG_ARR: {
            uint32_t n = read_u32(p);
            pad(); std::cout << "Array (" << n << " items):" << std::endl;
            for (uint32_t i = 0; i < n; ++i) {
                print_value(p, indent + 1);
            }
            break;
        }
        default:
            pad(); std::cout << "[Unknown tag: " << (int)tag << "]" << std::endl;
            break;
    }
}

// Parse command line into vector of strings
std::vector<std::string> parse_command(const std::string& line) {
    std::vector<std::string> args;
    std::istringstream iss(line);
    std::string token;
    bool in_quotes = false;
    std::string current_arg;

    while (iss >> std::quoted(current_arg)) {
        args.push_back(current_arg);
    }
    
    // Fallback for unquoted arguments
    if (args.empty() && !line.empty()) {
        std::istringstream iss_fallback(line);
        std::string arg;
        while(iss_fallback >> arg) {
            args.push_back(arg);
        }
    }
    
    return args;
}

// Send command and print response
bool send_command(SOCKET sock, const std::vector<std::string>& cmd) {
    if (cmd.empty()) return true;
    
    // Send request
    auto request = make_request(cmd);
    uint32_t len = (uint32_t)request.size();
    std::vector<uint8_t> full_request;
    full_request.insert(full_request.end(), (uint8_t*)&len, (uint8_t*)&len + 4);
    full_request.insert(full_request.end(), request.begin(), request.end());
    
    if (send(sock, (char*)full_request.data(), (int)full_request.size(), 0) == SOCKET_ERROR) {
        std::cerr << "send() failed" << std::endl;
        return false;
    }

    if (cmd[0] == "SUBSCRIBE" || cmd[0] == "subscribe") {



        std::cout << "Subscribed to channel(s). Waiting for messages..." << std::endl;


        while (true) {


            uint8_t header[4];


            int recvd = 0;


            while (recvd < 4) {


                int r = recv(sock, (char*)header + recvd, 4 - recvd, 0);


                if (r <= 0) {


                    std::cerr << "recv() header failed" << std::endl;


                    return false;


                }


                recvd += r;


            }





            uint32_t resp_len;


            memcpy(&resp_len, header, 4);


            if (resp_len > 0) {


                std::vector<uint8_t> response(resp_len);


                recvd = 0;


                while (recvd < (int)resp_len) {


                    int r = recv(sock, (char*)response.data() + recvd, resp_len - recvd, 0);


                    if (r <= 0) {


                        std::cerr << "recv() body failed" << std::endl;


                        return false;


                    }


                    recvd += r;


                }


                const uint8_t* p = response.data();


                print_value(p);


            }


        }


    }



    // Receive response
    uint8_t header[4];
    int recvd = 0;
    while (recvd < 4) {
        int r = recv(sock, (char*)header + recvd, 4 - recvd, 0);
        if (r <= 0) { 
            std::cerr << "recv() header failed" << std::endl; 
            return false; 
        }
        recvd += r;
    }
    
    uint32_t resp_len;
    memcpy(&resp_len, header, 4);
    if (resp_len > 0) {
        std::vector<uint8_t> response(resp_len);
        recvd = 0;
        while (recvd < (int)resp_len) {
            int r = recv(sock, (char*)response.data() + recvd, resp_len - recvd, 0);
            if (r <= 0) { 
                std::cerr << "recv() body failed" << std::endl; 
                return false; 
            }
            recvd += r;
        }
        const uint8_t* p = response.data();
        print_value(p);
    }
    return true;
}



int main(int argc, char* argv[]) {
    // Default connection settings
    std::string host = "127.0.0.1";
    int port = 6379;
    bool is_benchmark_mode = false; 


    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--benchmark") {
            is_benchmark_mode = true;
        }
        if (arg == "-h" || arg == "--host") {
            if (i + 1 < argc) host = argv[++i];
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) port = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [-h host] [-p port]" << std::endl;
            std::cout << "  -h, --host  Server host (default: 127.0.0.1)" << std::endl;
            std::cout << "  -p, --port  Server port (default: 6379)" << std::endl;
            return 0;
        }
    }

    // Initialize Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed" << std::endl;
        WSACleanup();
        return 1;
    }

    // Connect
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    std::cout << "Attempting to connect to " << host << ":" << port << std::endl;
    
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        std::cerr << "connect() failed to " << host << ":" << port << " (Error: " << error << ")" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "client connected to " << host << ":" << port << std::endl;

    // Interactive loop

    if (is_benchmark_mode) {
        run_benchmark(sock);
        closesocket(sock);
        WSACleanup();
        return 0;
    }

    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        
        if (line.empty()) continue;
        if (line == "quit" || line == "exit" || line == "close") {
            std::cout << "closing the client" << std::endl;
            break;
        }
        
        auto cmd = parse_command(line);
        if (!send_command(sock, cmd)) {
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
} 