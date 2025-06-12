#ifndef SERVER_H
#define SERVER_H

int server(int server_port, bool https, const char* cert_path, const char* key_path);

#endif // SERVER_H