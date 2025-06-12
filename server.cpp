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

#include <openssl/ssl.h>
#include <openssl/err.h>

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    ERR_free_strings();
    EVP_cleanup();
}

SSL_CTX* create_ssl_context() {
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);

    if (!ctx) {
        std::cerr << "Unable to create SSL context." << std::endl;
        ERR_print_errors_fp(stderr);
        return nullptr;
    }

    return ctx;
}

bool configure_ssl_context(SSL_CTX* ctx, const char* cert_path, const char* key_path) {
    if (SSL_CTX_use_certificate_file(ctx, cert_path, SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Error loading certificate" << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_path, SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Error loading private key" << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        std::cerr << "Private key does not match certificate" << std::endl;
        ERR_print_errors_fp(stderr);
        return false;
    }

    return true;
}

int server(
    const int server_port,
    const bool https,
    const char* cert_path,
    const char* key_path
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

    INIT_SOCKET();

    SSL_CTX* ssl_ctx = nullptr;

    if (https) {
        init_openssl();
        ssl_ctx = create_ssl_context();
        if (!ssl_ctx) {
            CLEANUP_SOCKET();
            return -1;
        }

        if (!configure_ssl_context(ssl_ctx, cert_path, key_path)) {
            SSL_CTX_free(ssl_ctx);
            cleanup_openssl();
            CLEANUP_SOCKET();
            return -1;
        }

        std::cout << "HTTPS mode enabled. Using certificate: " << cert_path
                  << " and key: " << key_path << std::endl;
    }

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

    std::cout << "Server is listening on port " << server_port << " ("
              << (https ? "HTTPS" : "HTTP") << ")" << std::endl;

    while (true) {
        if (std::cin.eof()) {
            break;
        }

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

        SSL* ssl = nullptr;

        if (https) {
            ssl = SSL_new(ssl_ctx);
            SSL_set_fd(ssl, client_socket);

            if (SSL_accept(ssl) <= 0) {
                std::cerr << "SSL accept failed." << std::endl;
                ERR_print_errors_fp(stderr);
                SSL_free(ssl);
                CLOSESOCKET(client_socket);
                continue;
            }

            std::cout << "SSL connection established with client "
                      << inet_ntoa(client_addr.sin_addr) << ":"
                      << ntohs(client_addr.sin_port) << std::endl;
        }

        char buffer[1024];
        int bytes_received;

        if (https) {
            bytes_received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        } else {
            bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        }

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string request(buffer);

            std::string url = request.substr(request.find(' ') + 1);
            url = url.substr(0, url.find(' '));
            std::cout << "Extracted URL: " << url << std::endl;

            std::string response = webpage_handler(url);

            if (https) {
                SSL_write(ssl, response.c_str(), static_cast<int>(response.length()));
            } else {
                send(client_socket, response.c_str(), static_cast<int>(response.length()), 0);
            }
        } else {
            std::cerr << "Failed to receive data from client." << std::endl;
        }

        if (https && ssl) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }

        CLOSESOCKET(client_socket);
    }

    if (https) {
        SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
    }

    CLOSESOCKET(server_socket);
    CLEANUP_SOCKET();
    return 0;
}
