#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    // Accept a connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        // Read the request
        int bytes_read = read(new_socket, buffer, BUFFER_SIZE);
        if (bytes_read == 0) {
            break; // Connection closed by client
        }
        printf("Message received: %s\n", buffer);

        // Process and respond to the request here
        char *response = "Server: Acknowledged your message.\n";
        send(new_socket, response, strlen(response), 0);
    }

    // Close the socket
    close(new_socket);
    close(server_fd);

    return 0;
}
