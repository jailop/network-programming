#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>  // Include this header for errno

#define SERVER_PORT 8080
#define SERVER_ADDR "127.0.0.1"
#define BUFFER_SIZE 1024
#define THREAD_COUNT 10

volatile unsigned long response_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

void* send_requests(void* arg) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);
    ssize_t bytes_sent;
    
    memset(buffer, 'A', sizeof(buffer)); // Fill buffer with dummy data

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Set the socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        close(sockfd);
        pthread_exit(NULL);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    fd_set read_fds, write_fds;
    struct timeval timeout;

    while (1) { // Infinite loop to keep sending requests
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(sockfd, &write_fds);

        // Set timeout for select()
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // Use select() to wait until the socket is ready for writing
        int activity = select(sockfd + 1, &read_fds, &write_fds, NULL, &timeout);
        if (activity < 0) {
            perror("select() error");
            close(sockfd);
            pthread_exit(NULL);
        }

        if (FD_ISSET(sockfd, &write_fds)) {
            // Send data to the server
            bytes_sent = sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, addr_len);
            if (bytes_sent < 0) {
                perror("sendto failed");
                close(sockfd);
                pthread_exit(NULL);
            }
        }

        // Simulate some delay
        usleep(50000); // 50 milliseconds
    }

    close(sockfd);
    pthread_exit(NULL);
}

void* report_responses(void* arg) {
    while (1) {
        sleep(1); // Report every second
        pthread_mutex_lock(&count_mutex);
        printf("Responses received: %lu\n", response_count);
        pthread_mutex_unlock(&count_mutex);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[THREAD_COUNT];
    pthread_t reporter_thread;

    // Create the reporter thread
    if (pthread_create(&reporter_thread, NULL, report_responses, NULL) != 0) {
        perror("Failed to create reporter thread");
        exit(EXIT_FAILURE);
    }

    // Create worker threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_create(&threads[i], NULL, send_requests, NULL) != 0) {
            perror("Failed to create worker thread");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_join(reporter_thread, NULL);

    return 0;
}

