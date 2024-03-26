#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

char nickname[32];

void *receive_messages(void *arg) {
    int socket = *(int *)arg;
    char message[BUFFER_SIZE];

    while (1) {
        int bytes_read = recv(socket, message, BUFFER_SIZE, 0);
        if (bytes_read <= 0) {
            break;
        }

        message[bytes_read] = '\0';
        printf("%s", message);
    }

    return NULL;
}

int main() {
    int socket_fd;
    struct sockaddr_in server_address;

    // Create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Failed to create socket");
        exit(1);
    }

    // Configure server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address / address not supported");
        exit(1);
    }

    // Connect to server
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to connect to server");
        exit(1);
    }

    // Get nickname from user
    printf("Enter your nickname: ");
    fgets(nickname, sizeof(nickname), stdin);
    nickname[strcspn(nickname, "\n")] = '\0'; // Remove trailing newline

    // Send nickname to server
    send(socket_fd, nickname, strlen(nickname), 0);

    // Start thread to receive messages
    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, &socket_fd);

    // Send messages
    while (1) {
        char message[BUFFER_SIZE];
        fgets(message, BUFFER_SIZE, stdin);

        if (strcmp(message, "/quit\n") == 0) {я
            break;
        }

        send(socket_fd, message, strlen(message), 0);
    }

    close(socket_fd);
    return 0;
}