//
// Created by merlin on 3/22/24.
//

#include "MyClient.h"

#include <utility>


// Function to search for a file in a directory and return a pointer to it
fs::path *find_file(const std::string &directory, const std::string &filename) {
    fs::path *result = nullptr;
    for (const auto &entry: fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().filename() == filename) {
            result = new fs::path(entry.path());
            break;
        }
    }
    return result;
}

long millis() {
    // Get the current time point
    auto now = std::chrono::system_clock::now();

    // Get the duration since the epoch (time point in milliseconds)
    auto duration = now.time_since_epoch();

    // Convert the duration to milliseconds
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return millis;
}


MyClient::MyClient(int client_socket, std::string directory_path) : _client_socket(client_socket),
                                                                    _directory_path(std::move(directory_path)) {
    std::cout << "Client connected" << std::endl;
    _timer = millis();
}

int MyClient::socket_ready() {
//    std::cout << "Socket ready" << std::endl;
//    std::cout << "Receive: " << _receive << std::endl;

    if (_receive) {
        char recv_buffer[MAX_DATA];
        ssize_t bytes_received;

        // Receive data from client
        bytes_received = recv(_client_socket, recv_buffer, MAX_DATA, 0);
        if (bytes_received < 0) {
            // Error receiving data (socket is likely not alive)
            return 1;
        } else if (bytes_received == 0) {
            // Connection closed (socket is not alive)
            return 2;
        }

        std::cout << "Receive" << std::endl;
        _timer = millis();

        // Print the received data
        std::cout << "Received from client: ";
        std::cout.write(recv_buffer, bytes_received);
        std::cout << std::endl;

        char const *cc = &recv_buffer[0];
        for (; (cc - &recv_buffer[0]) < bytes_received; cc++) {
            std::cout << cc[0] << std::endl;
            if (_c - &_buffer[0] > BUFFER_SIZE) {
                std::cerr << "Buffer overflow" << std::endl;
                return 1;
            }

            *_c = *cc;
            _c++;

            if (*cc == 0x4) {
                std::cout << "EOT" << std::endl;
                *_c = '\0';
                _receive = false;
                process_received_data();
                _c = &_buffer[0];
                std::cout << "Receive End" << std::endl;
                return 0;
            }
        }

        // Print full bugger
        std::cout << "Received buffer: ";
        std::cout.write(_buffer, strlen(_buffer));
        std::cout << std::endl;
    } else {
        std::cout << "Send" << std::endl;
        char send_buffer[MAX_DATA];

        char *cc = &send_buffer[0];
        for (; (cc - &send_buffer[0]) < MAX_DATA; cc++) {
            std::cout << _c[0] << std::endl;
            if (_c - &_buffer[0] > BUFFER_SIZE) {
                std::cerr << "Buffer overflow" << std::endl;
                exit(0);
                return 1;
            }

            *cc = *_c;

            if (*_c == '\0') {//&& _file_offset != -1
                fill_send_buffer();
                if (strlen(_buffer) == 0) {
                    std::cerr << "Empty file" << std::endl;
                    return 1;
                }

                _c = &_buffer[0];
                continue;
            }
            _c++;

            if (*cc == 0x4) {
                std::cout << "EOT" << std::endl;
                *_c = '\0';
                if (MAX_DATA - (cc - &send_buffer[0]) > 1) {
                    *(cc + 1) = '\0';
                }
                _receive = true;
                _c = &_buffer[0];
                std::cout << "Send End" << std::endl;
                break;
            }
        }
        std::cerr << "Buffer length: " << strlen(send_buffer) << std::endl;

        // Print sent data
        std::cout << "Send to the client: ";
        std::cout.write(send_buffer, strlen(send_buffer));
        std::cout << std::endl;

        send(_client_socket, send_buffer, strlen(send_buffer), 0);
    }
    return 0;
}


void MyClient::process_received_data() {
    // Print the data
    std::cout << "Processing data: ";
    std::cout.write(_buffer, _c - &_buffer[0]);
    std::cout << std::endl;

    // Process received data (e.g., save to a file)
    // Parse protocol
    std::string protocol;
    char const *cc = &_buffer[0];
    for (; *cc != '#' && cc < _c; cc++) {
        protocol += cc[0];
    }
    if (protocol != "PROTO:1.4.8.8") {
        // Send response to client
        std::cerr << "Invalid protocol" << std::endl;
        sprintf(_buffer, "Invalid protocol\x4");
        return;
    }
    cc++;

    // Parse command name
    std::string command; //(BUFFER_SIZE, 0x4)
    for (; *cc != '#' && cc < _c; cc++) {
        command += cc[0];
    }
    cc++;

//    std::cout << "Parsed command length: " << command.length() << std::endl;
    if (command.length() != (_c - cc)) {
        // Print parsed command
        std::cout << "Parsed command: " << command << std::endl;
    }

    if (command == "NEW") {
        std::cout << "NEW" << std::endl;

        // Parse file name
        std::string file_name;
        for (; *cc != '#' && cc < _c; cc++) {
            file_name += cc[0];
        }
        if (*(cc - 1) != 0x1C) {
            // Send response to client
            std::cerr << "Invalid filename" << std::endl;
            sprintf(_buffer, "Invalid filename\x4");
            return;
        }
        file_name.erase(file_name.size() - 1);
        cc++;

        // Print file name
        std::cout << "Parsed filename: " << file_name << std::endl;

        //Search for a file in search directory
        _file_path = find_file(_directory_path, file_name);
        if (_file_path == nullptr) {
            // Send response to client
            std::cerr << "File not found" << std::endl;
            sprintf(_buffer, "File not found\x4");
            return;
        }
        std::cout << "Found file: " << *_file_path << std::endl;

        uint64_t file_size = fs::file_size(*_file_path);
        std::cout << "File size: " << file_size << std::endl;

        // Send response to client
        sprintf(_buffer, "PROTO:1.4.8.8#NEW#OK#%s\x1C#%lu\x4", file_name.c_str(), file_size);
        return;
    } else if (command == "REC") {
        std::cout << "REC" << std::endl;

        if (_file_path == nullptr) {
            std::cerr << "File not ready" << std::endl;
            sprintf(_buffer, "File not ready\x4");
            return;
        }

        //Send raw file data
        if (std::ifstream input_file(*_file_path); !input_file.is_open()) {
            std::cerr << "Failed to open file" << std::endl;
            sprintf(_buffer, "PROTO:1.4.8.8#REC#ERR#%s\x4", "Failed to open file");
            return;
        }
        _file_offset = 0;

        // Send response to client
        sprintf(_buffer, "PROTO:1.4.8.8#REC#OK#");
        return;
    }


    // Send response to client
    sprintf(_buffer, "PROTO:1.4.8.8#REC#ERR#%s#\x4", "Invalid command");
}

void MyClient::fill_send_buffer() {
    std::cout << "fill buffer" << std::endl;

    std::ifstream input_file(*_file_path);
    if (!input_file.is_open()) {
        _buffer[0] = '\0';
        return;
    }

    input_file.seekg(_file_offset);
    input_file.read(&_buffer[0], BUFFER_SIZE - 1);
    _file_offset = input_file.tellg();

    // Check if we are at the end of the file
    if (input_file.eof()) {
        std::cout << "Reached end of file" << std::endl;
        _buffer[input_file.gcount()] = '\x4';
    } else {
        std::cout << "Not at end of file" << std::endl;
        _buffer[input_file.gcount()] = '\0';
    }

    input_file.close();

    std::cout << "g_count: " << input_file.gcount() << std::endl;
    std::cout << "file_offset: " << _file_offset << std::endl;
}

MyClient::~MyClient() {
    std::cout << "Client disconnected" << std::endl;
}

bool MyClient::is_receiving() const {
    return _receive;
}

bool MyClient::timeout() const {
    if (millis() - _timer > 10000) {
        return true;
    }
    return false;
}

