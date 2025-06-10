#include <iostream>
#include <string>
#include <fstream>
#include "server.h"
#include "yaml/yaml.hpp"

namespace std {
    class invalid_argument;
}

int main(const int argc, char* argv[]) {
    std::string config_file = "config.yaml";
    int server_port = 80;

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
    }

    return server(server_port);
}
