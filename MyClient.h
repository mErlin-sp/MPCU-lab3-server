//
// Created by merlin on 3/22/24.
//

#ifndef LAB3_SERVER_MYCLIENT_H
#define LAB3_SERVER_MYCLIENT_H

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>
#include <poll.h>
#include <csignal>

const int BUFFER_SIZE = 1024;

const int MAX_DATA = 5;

namespace fs = std::filesystem;

fs::path *find_file(const std::string &directory, const std::string &filename);

class MyClient {

public:
    explicit MyClient(int client_socket,std::string directory_path);

    ~MyClient();

    int socket_ready();

    bool is_receiving() const;

private:
    std::string _directory_path;

    fs::path *_file_path = nullptr;

    bool _receive = true;

    int _client_socket;

    char _buffer[BUFFER_SIZE]{};

    char *_c = &_buffer[0];

    void process_received_data();

};


#endif //LAB3_SERVER_MYCLIENT_H
