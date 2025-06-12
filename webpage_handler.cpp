#include "webpage_handler.h"
#include "mime_types_data.h"

#include <string>
#include <fstream>
#include <unordered_map>
#include <filesystem>

std::string content_type(const std::string &file_extension) {
    std::unordered_map<std::string, std::string> mime_types;

    std::istringstream csv_stream(embedded::MIME_TYPES_CSV);
    std::string line;

    std::getline(csv_stream, line);

    while (std::getline(csv_stream, line)) {
        std::stringstream line_stream(line);
        std::string extension, mime_type;

        std::getline(line_stream, extension, ',');
        std::getline(line_stream, mime_type, ',');

        mime_types[extension] = mime_type;
    }

    std::string ext;

    if (ext = file_extension; !ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }

    return mime_types.contains(ext) ? mime_types[ext] : "application/octet-stream";
}

std::string webpage_handler(
    const std::string &url
) {
    std::string mutable_url = url;
    auto extension = std::filesystem::path(mutable_url).extension().string();

    if (mutable_url == "/") {
        mutable_url = "/index.html";
    }

    if (!mutable_url.ends_with(".html")) {
        if (std::filesystem::exists(std::filesystem::path("www")) && std::filesystem::is_directory(std::filesystem::path("www"))) {
            if (std::filesystem::exists(std::filesystem::path(mutable_url + ".html")) && std::filesystem::is_regular_file(std::filesystem::path(mutable_url + ".html"))) {
                mutable_url += ".html";
            }
        }
    }

    std::ifstream content("www" + mutable_url);

    if (!content.is_open()) {
        if (std::ifstream fallback_content("www/404.html"); fallback_content.is_open()) {
            return "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n" +
                   std::string(std::istreambuf_iterator(fallback_content), std::istreambuf_iterator<char>());
        }

        return "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n" +
               std::string(
                   R"(<html><body style="background-color: black; margin: 0; display: flex; justify-content: center; align-items: center; height: 100vh;"><div style="text-align: center;"><h1 style="font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; color: white;">404</h1><p style="font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; color: white;">Page Not Found</p></div><p style="position: absolute; bottom: 0; left: 50%; transform: translateX(-50%); padding: 10px; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; color: white;">Powered by Jella Web Server</p></body></html>)");
    }
    return "HTTP/1.1 200 OK\r\nContent-Type: " + content_type(extension) + "\r\n\r\n" +
           std::string(std::istreambuf_iterator<char>(content), std::istreambuf_iterator<char>());
}