#include "server.h"
#include <iostream>
#include <format>
#include <string>
#include <winsock2.h>

#include "webpage_handler.h"

[[noreturn]] int server(
    const int server_port
) {
    if (server_port < 0 || server_port > 65535) {
        std::cerr << "Invalid port number. Please use a port between 0 and 65535." << std::endl;
        return -1;
    }

    if (server_port > 49151) {
        std::cerr <<
                "WARNING: These are Dynamic/Private/Ephemeral ports used by client applications when initiating a connection to a server. These ports are assigned dynamically for short-term use and are not registered for specific services."
                << std::endl;
    }

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return -1;
    }

    const SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind to port " << server_port << " failed." << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server is listening on port " << server_port << std::endl;

    while (true) {
        sockaddr_in client_addr;
        int client_addr_size = sizeof(client_addr);
        const SOCKET client_socket = accept(server_socket, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_size);

        if (client_socket == INVALID_SOCKET) {
            std::cerr << std::format("Client {}:{} accepting failure.\n", inet_ntoa(client_addr.sin_addr),
                                     ntohs(client_addr.sin_port));
            continue;
        }

        std::cout << std::format("Client {}:{} connected.", inet_ntoa(client_addr.sin_addr),
                                 ntohs(client_addr.sin_port)) << std::endl;

        char buffer[1024];
        if (const int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0); bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string request(buffer);

            std::string url = request.substr(request.find(' ') + 1);
            url = url.substr(0, url.find(' '));
            std::cout << "Extracted URL: " << url << std::endl;

            std::string response = webpage_handler(url);
            send(client_socket, response.c_str(), response.length(), 0);
        } else {
            std::cerr << "Failed to receive data from client." << std::endl;
        }

        closesocket(client_socket);
    }
}
