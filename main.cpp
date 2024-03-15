#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <csignal>
#include <chrono>
#include <filesystem>

const int BUFFER_SIZE = 1024;
// Flag to indicate if the program should continue running
volatile sig_atomic_t interrupted = 0;

namespace fs = std::filesystem;

std::string directoryPath;
fs::path *filePath;

// Signal handler for SIGINT
void signal_handler(int signal) {
    std::cout << "Interrupt signal (" << signal << ") received.\n";
    interrupted = 1;
}

auto millis() {
    // Get the current time point
    auto now = std::chrono::system_clock::now();

    // Get the duration since the epoch (time point in milliseconds)
    auto duration = now.time_since_epoch();

    // Convert the duration to milliseconds
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return millis;
}


// Function to search for a file in a directory and return a pointer to it
fs::path *findFile(const std::string &directory, const std::string &filename) {
    fs::path *result = nullptr;
    for (const auto &entry: fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().filename() == filename) {
            result = new fs::path(entry.path());
            break;
        }
    }
    return result;
}

int handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Receive data from client
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        // Error receiving data (socket is likely not alive)
        return 1;
    } else if (bytes_received == 0) {
        // Connection closed (socket is not alive)
        return 2;
    }

    // Print the received data
    std::cout << "Received from client: ";
    std::cout.write(buffer, bytes_received);
    std::cout << "/*END*/" << std::endl;

    // Process received data (e.g., save to a file)
    // Parse protocol
    std::string protocol;
    char const *c = &buffer[0];
    for (; *c != '#' && (c - &buffer[0]) < bytes_received; c++) {
        char cc = *c;
        protocol += cc;
    }
    if (protocol != "PROTO:1.4.8.8") {
        // Send response to client
        std::cout << "Invalid protocol" << std::endl;
        const char *response = "Invalid protocol";
        send(client_socket, response, strlen(response), 0);
        return 0;
    }
    c++;

    // Parse command name
    std::string command; //(BUFFER_SIZE, 0x4)
    for (; *c != '#' && (c - &buffer[0]) < bytes_received; c++) {
        char cc = *c;
        command += cc;
    }
    c++;

//    std::cout << "Parsed command length: " << command.length() << std::endl;
    if (command.length() != bytes_received) {
        // Print parsed command
        std::cout << "Parsed command: " << command << std::endl;
    }

    if (command == "NEW") {
        std::cout << "NEW" << std::endl;

        // Parse file name
        std::string file_name;
        for (; *c != '#' && (c - &buffer[0]) < bytes_received; c++) {
            char cc = *c;
            file_name += cc;
        }
        if (*(c - 1) != 0x1C) {
            // Send response to client
            std::cout << "Invalid filename" << std::endl;
            const char *response = "Invalid filename";
            send(client_socket, response, strlen(response), 0);
            return 0;
        }
        file_name.erase(file_name.size() - 1);
        c++;

        // Print file name
        std::cout << "Parsed filename: " << file_name << std::endl;

        //Search for a file in search directory

        filePath = findFile(directoryPath, file_name);
        if (filePath == nullptr) {
            // Send response to client
            std::cout << "File not found" << std::endl;
            const char *response = "File not found";
            send(client_socket, response, strlen(response), 0);
            return 0;

        }
        std::cout << "Found file: " << *filePath << std::endl;

        uint64_t fileSize = fs::file_size(*filePath);
        std::cout << "File size: " << fileSize << std::endl;

        // Send response to client
        std::string response = "PROTO:1.4.8.8#NEW#";
        response += file_name;
        response += '#';
        response += std::to_string(fileSize);
        response += (char) 0x4;

        send(client_socket, response.c_str(), strlen(response.c_str()), 0);

        return 0;
    }


    // Send response to client
    const char *response = "Invalid command";
    send(client_socket, response, strlen(response), 0);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << "./program <server_address> <server_port> <directory_path>" << std::endl;
        return 1;
    }

    // Install signal handler for SIGINT
    std::signal(SIGINT, signal_handler);

    // Parse command line arguments
    uint32_t serverAddress = std::stoi(argv[1]);
    uint16_t serverPort = std::stoi(argv[2]);
    directoryPath = argv[3];

    int server_socket;
    int client_socket;
    sockaddr_in server_addr{};
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        return 1;
    }

    // Bind socket to port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = serverAddress;
    server_addr.sin_port = serverPort;  //htons(PORT)
    if (bind(server_socket, (sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        perror("Error listening for connections");
        return 1;
    }

    std::cout << "Server started. Listening on port " << serverPort << " (" << ntohs(serverPort) << ")" << std::endl;

    // Accept incoming connections and handle them iteratively
    while (!interrupted) {
        client_socket = accept(server_socket, (sockaddr *) &client_addr, &client_addr_len);
        if (client_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            perror("Error accepting connection");
            continue;
        }

        std::cout << "Client connected" << std::endl;
        auto con_timeout = millis();

        while (!interrupted && millis() - con_timeout < 30000) {
            // Handle client request
            int err = handle_client(client_socket);
            if (err == 0) {
                con_timeout = millis();
            } else if (err == 2) {
                break;
            }
        }

        // Close client socket
        close(client_socket);
        delete filePath;
        filePath = nullptr;
        std::cout << "Client disconnected" << std::endl;
    }

    // Close server socket
    close(server_socket);

    return 0;
}
