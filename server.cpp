#include "server.h"
#include <iostream>
#include <string>
#include "webpage_handler.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using socket_t = SOCKET;
    #define CLOSESOCKET closesocket
    #define INIT_SOCKET() \
        WSADATA wsaData; \
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { \
            std::cerr << "WSAStartup failed." << std::endl; \
            return -1; \
        }
    #define CLEANUP_SOCKET() WSACleanup()
#else
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using socket_t = int;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define CLOSESOCKET close
#define INIT_SOCKET()
#define CLEANUP_SOCKET()
#endif

int server(const int server_port) {
    if (server_port < 0 || server_port > 65535) {
        std::cerr << "Invalid port number. Please use a port between 0 and 65535." << std::endl;
        return -1;
    }

    if (server_port > 49151) {
        std::cerr <<
                "WARNING: These are Dynamic/Private/Ephemeral ports used by client applications when initiating a connection to a server. These ports are assigned dynamically for short-term use and are not registered for specific services."
                << std::endl;
    }

    INIT_SOCKET();

    const socket_t server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        CLEANUP_SOCKET();
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&opt), sizeof(opt));

    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind to port " << server_port << " failed." << std::endl;
        CLOSESOCKET(server_socket);
        CLEANUP_SOCKET();
        return -1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        CLOSESOCKET(server_socket);
        CLEANUP_SOCKET();
        return -1;
    }

    std::cout << "Server is listening on port " << server_port << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_addr_size = sizeof(client_addr);
        const socket_t client_socket = accept(server_socket, reinterpret_cast<sockaddr *>(&client_addr),
                                              &client_addr_size);

        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Client " << inet_ntoa(client_addr.sin_addr) << ":"
                    << ntohs(client_addr.sin_port) << " accepting failure." << std::endl;
            continue;
        }

        std::cout << "Client " << inet_ntoa(client_addr.sin_addr) << ":"
                << ntohs(client_addr.sin_port) << " connected." << std::endl;

        char buffer[1024];
        if (const int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0); bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string request(buffer);

            std::string url = request.substr(request.find(' ') + 1);
            url = url.substr(0, url.find(' '));
            std::cout << "Extracted URL: " << url << std::endl;

            std::string response = webpage_handler(url);
            send(client_socket, response.c_str(), static_cast<int>(response.length()), 0);
        } else {
            std::cerr << "Failed to receive data from client." << std::endl;
        }

        CLOSESOCKET(client_socket);
    }
}
