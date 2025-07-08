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

std::vector<uint8_t> make_request(const std::vector<std::string>& args) {
    std::vector<uint8_t> buf;
    append_u32(buf, (uint32_t)args.size());
    for (const auto& arg : args) {
        append_str(buf, arg);
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
            
            // Special formatting for keys command (array of strings)
            bool is_keys_command = true;
            const uint8_t* temp_p = p;
            for (uint32_t i = 0; i < n; ++i) {
                if (temp_p[0] != TAG_STR) {
                    is_keys_command = false;
                    break;
                }
                temp_p++; // consume tag
                uint32_t len = read_u32(temp_p);
                temp_p += len; // skip string data
            }
            
            if (is_keys_command && n > 0) {
                // Special formatting for keys command
                pad(); std::cout << "Found " << n << " key" << (n == 1 ? "" : "s") << ":" << std::endl;
                for (uint32_t i = 0; i < n; ++i) {
                    p++; // consume TAG_STR
                    std::string s = read_str(p);
                    pad(); std::cout << "  " << (i + 1) << ") \"" << s << "\"" << std::endl;
                }
            } else if (n == 0) {
                pad(); std::cout << "No keys found." << std::endl;
            } else {
                // General array formatting
                pad(); std::cout << "Array[" << n << "]: [" << std::endl;
                for (uint32_t i = 0; i < n; ++i) {
                    if (i > 0) std::cout << "," << std::endl;
                    print_value(p, indent + 1);
                }
                pad(); std::cout << "]" << std::endl;
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
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }
    return args;
}

// Send command and receive response
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
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
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