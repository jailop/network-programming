#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_EVENTS 1000

volatile unsigned long request_count = 0;
volatile unsigned long response_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

void* report_requests(void* arg) {
    while (1) {
        sleep(1);
        pthread_mutex_lock(&count_mutex);
        printf("%lu %lu %.3f\n", request_count, response_count,
            response_count * 100.0 / request_count);
        request_count = 0;
        response_count = 0;
        pthread_mutex_unlock(&count_mutex);
    }
    pthread_exit(NULL);
}

int main() {
    int sockfd, epfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);
    struct epoll_event ev, events[MAX_EVENTS];
    pthread_t reporter_thread;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if ((epfd = epoll_create1(0)) < 0) {
        perror("epoll_create1 failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        perror("epoll_ctl failed");
        close(sockfd);
        close(epfd);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&reporter_thread, NULL, report_requests, NULL) != 0) {
        perror("Failed to create reporter thread");
        close(sockfd);
        close(epfd);
        exit(EXIT_FAILURE);
    }
    printf("Server is running on port %d\n", PORT);
    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            perror("epoll_wait failed");
            break;
        }
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == sockfd) {
                ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
                if (n < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        // perror("recvfrom failed");
                    }
                } else {
                    pthread_mutex_lock(&count_mutex);
                    request_count++;
                    pthread_mutex_unlock(&count_mutex);
                    // Echo the received data back to the client
                    ssize_t sent = sendto(sockfd, buffer, n, 0, (const struct sockaddr *)&client_addr, addr_len);
                    if (sent < 0) {
                        // perror("sendto failed");
                    } else {
                        pthread_mutex_lock(&count_mutex);
                        response_count++;
                        pthread_mutex_unlock(&count_mutex);
                    }
                }
            }
        }
    }
    close(epfd);
    close(sockfd);
    return 0;
}
