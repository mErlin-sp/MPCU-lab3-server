#include "MyClient.h"


// Flag to indicate if the program should continue running
volatile sig_atomic_t interrupted = 0;

std::string directory_path;

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
        std::cerr << "Invalid protocol" << std::endl;
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
        filePath = find_file(directory_path, file_name);
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
        std::string response = "PROTO:1.4.8.8#NEW#OK#";
        response += file_name;
        response += (char) 0x1C;
        response += '#';
        response += std::to_string(fileSize);
        response += (char) 0x4;

        send(client_socket, response.c_str(), strlen(response.c_str()), 0);

        return 0;
    } else if (command == "REC") {
        std::cout << "REC" << std::endl;

        if (filePath == nullptr) {
            std::cout << "File not ready" << std::endl;
            // Send response to client
            std::string response = "PROTO:1.4.8.8#REC#ERR#";
            response += "File not ready";
            response += (char) 0x4;

            send(client_socket, response.c_str(), strlen(response.c_str()), 0);

            return 0;
        }

        //Send raw file data
        std::ifstream inputFile(*filePath);
        if (!inputFile.is_open()) {
            std::cout << "Failed to open file" << std::endl;

            // Send response to client
            std::string response = "PROTO:1.4.8.8#REC#ERR#";
            response += "Failed to open file";
            response += (char) 0x4;

            send(client_socket, response.c_str(), strlen(response.c_str()), 0);
            return 0;
        }

        // Send response to client
        const char *response_start = "PROTO:1.4.8.8#REC#OK#";
        send(client_socket, response_start, strlen(response_start), 0);

        char response_buffer[BUFFER_SIZE];

        do {
            inputFile.read(&response_buffer[0], BUFFER_SIZE);
            send(client_socket, response_buffer, inputFile.gcount(), 0);
//            std::cout << "gcount: " << inputFile.gcount() << std::endl;
        } while (inputFile.gcount() > 0);

        char end = 0x4;
        send(client_socket, &end, 1, 0);
        std::cout << "SENT" << std::endl;

        return 0;
    }


    // Send response to client
    const char *response = "Invalid command";
    send(client_socket, response, strlen(response), 0);

    return 0;
}

void print_usage() {
    std::cerr << "Usage: " << "./program <server_address> <server_port> <directory_path> [-p] [-m]" << std::endl;
}

int main(int argc, char *argv[]) {
    // Install signal handler for SIGINT
    std::signal(SIGINT, signal_handler);

    int opt;

    int parallel = 0;
    int maxClients = 1; // Maximum number of concurrent clients
    pid_t pid;

    while ((opt = getopt(argc, argv, ":p:m:")) != -1) {
//        std::cout << "opt: " << std::to_string(opt) << std::endl;
//        std::cout << "opt_arg: " << optarg << std::endl;
        switch (opt) {
            case 'm':
                maxClients = std::stoi(optarg);
                if (maxClients == 0) {
                    print_usage();
                    return 1;
                }
                break;
            case 'p':
                if (*optarg > '4') {
                    print_usage();
                    return 1;
                }
                parallel = *optarg - '0';
                break;
            default:
                print_usage();
                return 1;
        }
    }

    std::cout << "parallel: " << std::to_string(parallel) << std::endl;
    std::cout << "max clients: " << std::to_string(maxClients) << std::endl;

    if (argc - optind != 3) {
        print_usage();
        return 1;
    }

    // Parse command line arguments
    uint32_t serverAddress = std::stoi(argv[optind++]);
    uint16_t serverPort = std::stoi(argv[optind++]);
    directory_path = argv[optind++];

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
    if (listen(server_socket, parallel ? maxClients : 1) < 0) {
        perror("Error listening for connections");
        return 1;
    }

    std::cout << "Server started. Listening on port " << serverPort << " (" << ntohs(serverPort) << ")" << std::endl;

    if (parallel <= 2) {
        std::cout << "parallel2" << std::endl;

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

            if (parallel) {
                // Fork a new process to handle the client connection
                pid = fork();
                if (pid < 0) {
                    std::cerr << "Failed to fork." << std::endl;
                    close(client_socket);
                    continue;
                } else if (pid == 0) {
                    // Child process: handle client connection
                    std::cout << "Child process. PID: " << pid << std::endl;
                    close(server_socket); // Close the server socket in the child process
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

                    exit(0); // Terminate the child process
                } else {
                    // Parent process: close client socket and continue accepting new connections
                    std::cout << "Parent process. PID: " << pid << std::endl;
                    close(client_socket);

                    // Wait for any terminated child processes to prevent zombies
                    while (waitpid(-1, NULL, WNOHANG) > 0);
                }
            } else {
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
        }
    } else if (parallel <= 3) {
        std::cout << "parallel3" << std::endl;

        for (int i = 0; i < maxClients; ++i) {
            pid = fork();
            if (pid < 0) {
                std::cerr << "Failed to fork." << std::endl;
                return 1;
            } else if (pid == 0) {
                // Child process: handle client connections
                std::cout << "Child process. PID: " << pid << std::endl;
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
                exit(0);
            }
        }

        // Parent process: wait for all child processes to exit
        std::cout << "Parent process. PID: " << pid << std::endl;
        for (int i = 0; i < maxClients; ++i) {
            wait(nullptr);
        }
    } else if (parallel <= 4) {
        std::cout << "parallel4" << std::endl;

        std::vector<MyClient> client_fds;
        fd_set read_fds;
        pollfd pollfds[maxClients + 1];

        pollfds[0].fd = server_socket;
        pollfds[0].events = POLLIN | POLLOUT;

        while (!interrupted) {
            int n_ready = poll(pollfds, client_fds.size() + 1, -1);
            if (n_ready < 0) {
                perror("poll");
                continue;
            }

            if (pollfds[0].revents & POLLIN) {
                // Accept the incoming connection
                client_socket = accept(server_socket, (sockaddr *) &client_addr, &client_addr_len);
                if (client_socket < 0) {
                    perror("Error accepting connection");
                    continue;
                }

                client_fds.emplace_back(client_socket, directory_path);
                pollfds[client_fds.size()].fd = client_socket;
                pollfds[client_fds.size()].events = POLLIN;

            }

            std::cout << "client_fds.size(): " << client_fds.size() << std::endl;
            for (int i = 0; i <= client_fds.size(); ++i) {
                std::cout << "i: " << i << std::endl;
                if (pollfds[i + 1].revents & POLLIN & client_fds[i].is_receiving()) {
                    std::cout << "poll_in: " << i << std::endl;
                    if (client_fds[i].socket_ready() != 0) {
                        close(pollfds[i + 1].fd);
                        client_fds.erase(client_fds.begin() + i);
                    }
                } else if (pollfds[i + 1].revents & POLLOUT & !client_fds[i].is_receiving()) {
                    std::cout << "poll_out: " << i << std::endl;
                    if (client_fds[i].socket_ready() != 0) {
                        close(pollfds[i + 1].fd);
                        client_fds.erase(client_fds.begin() + i);
                    }
                }
            }
        }

//        fd_set read_fds;
//        int max_sd;
//        struct timeval timeout{};
//
//        // Initialize the file descriptor set
//        FD_ZERO(&read_fds);
//        FD_SET(server_socket, &read_fds);
//        max_sd = server_socket;
//
//        while (!interrupted) {
//            // Set the timeout
//            timeout.tv_sec = 1;
//            timeout.tv_usec = 0;
//
//            // Wait for activity on any of the sockets
//            if ((select(max_sd + 1, &read_fds, nullptr, nullptr, &timeout) < 0) && (errno != EINTR)) {
//                perror("Error in select");
//                break;
//            }
//
//            if (FD_ISSET(server_socket, &read_fds)) {
//                // Accept the incoming connection
//                client_socket = accept(server_socket, (sockaddr *) &client_addr, &client_addr_len);
//                if (client_socket < 0) {
//                    perror("Error accepting connection");
//                    break;
//                }
//
//                std::cout << "Client connected" << std::endl;
//
//                // Fork a new process to handle the client connection
//                pid = fork();
//                if (pid < 0) {
//                    perror("Error forking process");
//                    break;
//                } else if (pid == 0) {
//                    // Child process: handle client connection
//                    close(server_socket); // Close the server socket in the child process
//                    handle_client(client_socket);
//                    close(client_socket);
//                    std::cout << "Client disconnected" << std::endl;
//                    return 0;
//                } else {
//                    // Parent process: close client socket and continue accepting new connections
//                    close(client_socket);
//                }
//
//            }
//        }
    } else {
        std::cerr << "Invalid server type!" << std::endl;
        return 1;
    }

    // Close server socket
    close(server_socket);

    return 0;
}
