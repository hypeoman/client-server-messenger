#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int client_sockets[MAX_CLIENTS];
char client_names[MAX_CLIENTS][32];
int num_clients = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    int client_index = -1;

    // Receive client name
    char name[32];
    recv(client_socket, name, sizeof(name), 0);
    name[strcspn(name, "\n")] = '\0'; // Remove trailing newline

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = client_socket;
            client_index = i;
            strcpy(client_names[i], name);
            num_clients++;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    if (client_index == -1) {
        printf("Server is full!\n");
        close(client_socket);
        return NULL;
    }

    // Broadcast welcome message
    char message[BUFFER_SIZE];
    sprintf(message, "%s has joined the chat.\n", name);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0 && i != client_index) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }

    // Handle client messages
    while (1) {
        int bytes_read = recv(client_socket, message, BUFFER_SIZE, 0);
        if (bytes_read <= 0) {
            break;
        }

        message[bytes_read] = '\0';

        // Get timestamp
        time_t now = time(NULL);
        struct tm *tm_struct = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%m.%d.%y %H.%M.%S", tm_struct);

        // Format message with timestamp and nickname
        char formatted_message[BUFFER_SIZE];
        sprintf(formatted_message, "<%s> %s: %s", timestamp, name, message);

        // Broadcast message to all clients
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != 0) {
                send(client_sockets[i], formatted_message, strlen(formatted_message), 0);
            }
        }
        pthread_mutex_unlock(&mutex);
    }

    // Client disconnected
    pthread_mutex_lock(&mutex);
    client_sockets[client_index] = 0;
    num_clients--;
    pthread_mutex_unlock(&mutex);

    sprintf(message, "%s has left the chat.\n", name);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_in server_address;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        exit(1);
    }

    // Configure server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to bind socket");
        exit(1);
    }

    // Listen for connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Failed to listen on socket");
        exit(1);
    }

    printf("Server started on port %d\n", PORT);

    // Accept connections
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_size = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_size);
        if (client_socket == -1) {
            perror("Failed to accept connection");
            continue;
        }

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, &client_socket);
    }

    close(server_socket);
    return 0;
}