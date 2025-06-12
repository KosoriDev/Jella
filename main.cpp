#include <iostream>
#include <string>
#include <fstream>
#include "server.h"
#include "yaml/Yaml.hpp"

namespace std {
    class invalid_argument;
}

int main(const int argc, char* argv[]) {
    std::string config_file = "config.yaml";
    int server_port = 80;
    bool https = false;

    auto cert_path = "server.crt";
    auto key_path = "server.key";

    for (int i = 1; i < argc; ++i) {
        if (std::string arg = argv[i]; (arg == "--config" || arg == "-c") && i + 1 < argc) {
            config_file = argv[++i];
        }
    }

    if (!std::ifstream(config_file)) {
        std::cout << "Configuration file not found, continuing with default settings.\n";
    } else {
        Yaml::Node root;
        Yaml::Parse(root, config_file.c_str());
        if (root["port"].IsScalar()) {
            try {
                server_port = std::stoi(root["port"].As<std::string>());
                https = root["https"].As<bool>(false);
                cert_path = const_cast<char*>(root["cert"].As<std::string>("server.crt").c_str());
                key_path = const_cast<char*>(root["key"].As<std::string>("server.key").c_str());
            }
            catch (const std::exception& e) {
                std::cerr << "Error parsing port number: " << e.what() << "\n";
            }
        }
    }

    for (int i = 1; i < argc; ++i) {
        if (std::string arg = argv[i]; (arg == "--port" || arg == "-p") && i + 1 < argc) {
            server_port = std::stoi(argv[++i]);
        }

        if (std::string arg = argv[i]; (arg == "--https" || arg == "-s") && i + 1 < argc) {
            https = (std::string(argv[++i]) == "true" || std::string(argv[i]) == "1");
        }

        if (std::string arg = argv[i]; (arg == "--cert" || arg == "-c") && i + 1 < argc) {
            cert_path = argv[++i];
        }

        if (std::string arg = argv[i]; (arg == "--key" || arg == "-k") && i + 1 < argc) {
            key_path = argv[++i];
        }
    }

    return server(server_port, https, cert_path, key_path);
}
