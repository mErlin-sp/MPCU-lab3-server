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


MyClient::MyClient(int client_socket, std::string directory_path) : _client_socket(client_socket),
                                                                    _directory_path(std::move(directory_path)) {
    std::cout << "Client connected" << std::endl;
}

int MyClient::socket_ready() {
    std::cout << "Socket ready" << std::endl;
//    std::cout << "Receive: " << _receive << std::endl;

    if (_receive) {
        std::cout << "Receive" << std::endl;

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
                return 1;
            }

            *cc = *_c;
            _c++;

            if (*_c == 0x4) {
                std::cout << "EOT" << std::endl;
                *_c = '\0';
                _receive = true;
                _c = &_buffer[0];
                break;
            }
        }

        // Print sent data
        std::cout << "Send to the client: ";
        std::cout.write(send_buffer, strlen(send_buffer));
        std::cout << std::endl;

        send(_client_socket, send_buffer, strlen(send_buffer), 0);
    }
    return 0;
}

//int handle_client(int client_socket) {
//    char buffer[BUFFER_SIZE];
//    ssize_t bytes_received;
//
//    // Receive data from client
//    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
//    if (bytes_received < 0) {
//        // Error receiving data (socket is likely not alive)
//        return 1;
//    } else if (bytes_received == 0) {
//        // Connection closed (socket is not alive)
//        return 2;
//    }
//
//    // Print the received data
//    std::cout << "Received from client: ";
//    std::cout.write(buffer, bytes_received);
//    std::cout << "/*END*/" << std::endl;
//
//    // Process received data (e.g., save to a file)
//    // Parse protocol
//    std::string protocol;
//    char const *c = &buffer[0];
//    for (; *c != '#' && (c - &buffer[0]) < bytes_received; c++) {
//        char cc = *c;
//        protocol += cc;
//    }
//    if (protocol != "PROTO:1.4.8.8") {
//        // Send response to client
//        std::cerr << "Invalid protocol" << std::endl;
//        const char *response = "Invalid protocol";
//        send(client_socket, response, strlen(response), 0);
//        return 0;
//    }
//    c++;
//
//    // Parse command name
//    std::string command; //(BUFFER_SIZE, 0x4)
//    for (; *c != '#' && (c - &buffer[0]) < bytes_received; c++) {
//        char cc = *c;
//        command += cc;
//    }
//    c++;
//
////    std::cout << "Parsed command length: " << command.length() << std::endl;
//    if (command.length() != bytes_received) {
//        // Print parsed command
//        std::cout << "Parsed command: " << command << std::endl;
//    }
//
//    if (command == "NEW") {
//        std::cout << "NEW" << std::endl;
//
//        // Parse file name
//        std::string file_name;
//        for (; *c != '#' && (c - &buffer[0]) < bytes_received; c++) {
//            char cc = *c;
//            file_name += cc;
//        }
//        if (*(c - 1) != 0x1C) {
//            // Send response to client
//            std::cout << "Invalid filename" << std::endl;
//            const char *response = "Invalid filename";
//            send(client_socket, response, strlen(response), 0);
//            return 0;
//        }
//        file_name.erase(file_name.size() - 1);
//        c++;
//
//        // Print file name
//        std::cout << "Parsed filename: " << file_name << std::endl;
//
//        //Search for a file in search directory
//        _file_path = find_file(_directory_path, file_name);
//        if (filePath == nullptr) {
//            // Send response to client
//            std::cout << "File not found" << std::endl;
//            const char *response = "File not found";
//            send(client_socket, response, strlen(response), 0);
//            return 0;
//
//        }
//        std::cout << "Found file: " << *filePath << std::endl;
//
//        uint64_t fileSize = fs::file_size(*filePath);
//        std::cout << "File size: " << fileSize << std::endl;
//
//        // Send response to client
//        std::string response = "PROTO:1.4.8.8#NEW#OK#";
//        response += file_name;
//        response += (char) 0x1C;
//        response += '#';
//        response += std::to_string(fileSize);
//        response += (char) 0x4;
//
//        send(client_socket, response.c_str(), strlen(response.c_str()), 0);
//
//        return 0;
//    } else if (command == "REC") {
//        std::cout << "REC" << std::endl;
//
//        if (filePath == nullptr) {
//            std::cout << "File not ready" << std::endl;
//            // Send response to client
//            std::string response = "PROTO:1.4.8.8#REC#ERR#";
//            response += "File not ready";
//            response += (char) 0x4;
//
//            send(client_socket, response.c_str(), strlen(response.c_str()), 0);
//
//            return 0;
//        }
//
//        //Send raw file data
//        std::ifstream inputFile(*filePath);
//        if (!inputFile.is_open()) {
//            std::cout << "Failed to open file" << std::endl;
//
//            // Send response to client
//            std::string response = "PROTO:1.4.8.8#REC#ERR#";
//            response += "Failed to open file";
//            response += (char) 0x4;
//
//            send(client_socket, response.c_str(), strlen(response.c_str()), 0);
//            return 0;
//        }
//
//        // Send response to client
//        const char *response_start = "PROTO:1.4.8.8#REC#OK#";
//        send(client_socket, response_start, strlen(response_start), 0);
//
//        char response_buffer[BUFFER_SIZE];
//
//        do {
//            inputFile.read(&response_buffer[0], BUFFER_SIZE);
//            send(client_socket, response_buffer, inputFile.gcount(), 0);
////            std::cout << "gcount: " << inputFile.gcount() << std::endl;
//        } while (inputFile.gcount() > 0);
//
//        char end = 0x4;
//        send(client_socket, &end, 1, 0);
//        std::cout << "SENT" << std::endl;
//
//        return 0;
//    }
//
//
//    // Send response to client
//    const char *response = "Invalid command";
//    send(client_socket, response, strlen(response), 0);
//
//    return 0;
//}


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
        std::ifstream inputFile(*_file_path);
        if (!inputFile.is_open()) {
            std::cerr << "Failed to open file" << std::endl;
            sprintf(_buffer, "PROTO:1.4.8.8#REC#ERR#%s\x4", "Failed to open file");
            return;
        }

        // Send response to client
        sprintf(_buffer, "PROTO:1.4.8.8#REC#OK#\x4");
        return;

//        // Send response to client
//        const char *response_start = "PROTO:1.4.8.8#REC#OK#";
//        send(client_socket, response_start, strlen(response_start), 0);
//
//        char response_buffer[BUFFER_SIZE];
//
//        do {
//            inputFile.read(&response_buffer[0], BUFFER_SIZE);
//            send(client_socket, response_buffer, inputFile.gcount(), 0);
////            std::cout << "gcount: " << inputFile.gcount() << std::endl;
//        } while (inputFile.gcount() > 0);
//
//        char end = 0x4;
//        send(client_socket, &end, 1, 0);
//        std::cout << "SENT" << std::endl;
//
//        return 0;
    }


    // Send response to client
    sprintf(_buffer, "PROTO:1.4.8.8#REC#ERR#%s#\x4", "Invalid command");
}

MyClient::~MyClient() {
    std::cout << "Client disconnected" << std::endl;
}

bool MyClient::is_receiving() const {
    return _receive;
}

