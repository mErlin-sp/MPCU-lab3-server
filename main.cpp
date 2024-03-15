#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <csignal>

const int BUFFER_SIZE = 1024;

// Flag to indicate if the program should continue running
volatile sig_atomic_t interrupted = 0;

// Signal handler for SIGINT
void signal_handler(int signal) {
    std::cout << "Interrupt signal (" << signal << ") received.\n";
    interrupted = 1;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Receive data from client
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        perror("Error receiving data from client");
        return;
    }

    // Process received data (e.g., save to a file)
    // For simplicity, we'll just print the received data
    std::cout << "Received from client: " << buffer << std::endl;

    // Send response to client
    const char *response = "Message received";
    send(client_socket, response, strlen(response), 0);

    // Close client socket
    close(client_socket);
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
    std::string directoryPath = argv[3];

    int server_socket;
    int client_socket;
    sockaddr_in server_addr{};
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
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

    std::cout << "Server started. Listening on port " << serverPort << std::endl;

    // Accept incoming connections and handle them iteratively
    while (!interrupted) {
        client_socket = accept(server_socket, (sockaddr *) &client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Error accepting connection");
            return 1;
        }

        std::cout << "Client connected" << std::endl;

        // Handle client request
        handle_client(client_socket);
    }

    // Close server socket
    close(server_socket);

    return 0;
}
