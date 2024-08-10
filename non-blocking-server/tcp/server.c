#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 10000

// Global variable to count the number of requests
volatile unsigned long request_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to handle a single client connection
void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    ssize_t n;

    while ((n = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        send(client_sock, buffer, n, 0);
        
        // Increment the request count
        pthread_mutex_lock(&count_mutex);
        request_count++;
        pthread_mutex_unlock(&count_mutex);
    }

    close(client_sock);
    pthread_exit(NULL);
}

// Function to periodically report the number of requests handled
void* report_requests(void* arg) {
    while (1) {
        sleep(1); // Report every second
        pthread_mutex_lock(&count_mutex);
        printf("Requests handled: %lu\n", request_count);
        request_count = 0; // Reset the count after reporting
        pthread_mutex_unlock(&count_mutex);
    }
    pthread_exit(NULL);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t reporter_thread;

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, MAX_CONNECTIONS) < 0) {
        perror("listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Start the reporter thread
    if (pthread_create(&reporter_thread, NULL, report_requests, NULL) != 0) {
        perror("Failed to create reporter thread");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d\n", PORT);

    while (1) {
        // Accept a new connection
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
            perror("accept failed");
            continue;
        }

        // Handle the new client in a separate thread
        int* client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_sock;
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, client_sock_ptr) != 0) {
            perror("Failed to create client thread");
            close(client_sock);
            free(client_sock_ptr);
            continue;
        }

        pthread_detach(client_thread); // Detach thread to prevent memory leak
    }

    close(server_sock);
    return 0;
}

