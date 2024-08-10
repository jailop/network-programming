#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>

#define SERVER_PORT 9999
#define SERVER_ADDR "192.168.1.175"
#define BUFFER_SIZE 1024
#define THREAD_COUNT 4

volatile unsigned long response_count = 0;
volatile unsigned long request_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

void* send_requests(void* arg) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t n;
    while (1) {
        memset(buffer, '0', sizeof(buffer));
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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
        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Connection failed");
            close(sockfd);
            // pthread_exit(NULL);
            sleep(1);
            continue;
        }
        while (1) {
            send(sockfd, buffer, sizeof(buffer), 0);
            pthread_mutex_lock(&count_mutex);
            request_count++;
            pthread_mutex_unlock(&count_mutex);
            n = recv(sockfd, buffer, BUFFER_SIZE, 0);
            if (n < 0) {
                perror("recv failed");
                break;
            } else if (n == 0) {
                break;
            } else {
                pthread_mutex_lock(&count_mutex);
                response_count++;
                pthread_mutex_unlock(&count_mutex);
            }
        }
        close(sockfd);
    }
    pthread_exit(NULL);
}

void* report_responses(void* arg) {
    while (1) {
        sleep(1);
        pthread_mutex_lock(&count_mutex);
        printf("sent %lu  recv %lu\n", request_count, response_count);
        request_count = 0;
        response_count = 0;
        pthread_mutex_unlock(&count_mutex);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[THREAD_COUNT];
    pthread_t reporter_thread;
    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_create(&threads[i], NULL, send_requests, NULL) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_create(&reporter_thread, NULL, report_responses, NULL) != 0) {
        perror("Failed to create reporter thread");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_join(reporter_thread, NULL);
    return 0;
}
