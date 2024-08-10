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
#define MAX_EVENTS 100000
#define THREAD_POOL_SIZE 12

// Global variable to count the number of requests
volatile unsigned long request_count = 0;
volatile unsigned long response_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

// Structure for thread pool tasks
typedef struct {
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    ssize_t length;
} thread_task_t;

pthread_mutex_t task_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t task_queue_cond = PTHREAD_COND_INITIALIZER;
thread_task_t* task_queue[MAX_EVENTS];
int task_queue_head = 0, task_queue_tail = 0, task_queue_size = 0;

// Function to periodically report the number of requests handled
void* report_requests(void* arg) {
    while (1) {
        sleep(1); // Report every second
        pthread_mutex_lock(&count_mutex);
        printf("%lu %lu %.3f\n", request_count, response_count,
            response_count * 100.0 / request_count);
        request_count = 0;
        response_count = 0;
        pthread_mutex_unlock(&count_mutex);
    }
    pthread_exit(NULL);
}

// Worker thread function
void* worker_thread(void* arg) {
    while (1) {
        thread_task_t* task;

        // Wait for tasks to be available
        pthread_mutex_lock(&task_queue_mutex);
        while (task_queue_size == 0) {
            pthread_cond_wait(&task_queue_cond, &task_queue_mutex);
        }

        // Get the next task from the queue
        task = task_queue[task_queue_head];
        task_queue_head = (task_queue_head + 1) % MAX_EVENTS;
        task_queue_size--;
        pthread_mutex_unlock(&task_queue_mutex);

        // Process the task
        if (sendto(task->sockfd, task->buffer, task->length, 0, (const struct sockaddr*)&task->client_addr, task->addr_len) < 0) {
            // perror("sendto failed");
        } else {

        // Increment the request count
            pthread_mutex_lock(&count_mutex);
            response_count++;
            pthread_mutex_unlock(&count_mutex);
        }

        // Free task
        free(task);
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
    pthread_t thread_pool[THREAD_POOL_SIZE];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Make socket non-blocking
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

    // Initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Create epoll instance
    if ((epfd = epoll_create1(0)) < 0) {
        perror("epoll_create1 failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Add socket to epoll
    ev.events = EPOLLIN; // Monitor for read events
    ev.data.fd = sockfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        perror("epoll_ctl failed");
        close(sockfd);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    // Start the reporter thread
    if (pthread_create(&reporter_thread, NULL, report_requests, NULL) != 0) {
        perror("Failed to create reporter thread");
        close(sockfd);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    // Create worker threads
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&thread_pool[i], NULL, worker_thread, NULL) != 0) {
            perror("Failed to create worker thread");
            close(sockfd);
            close(epfd);
            exit(EXIT_FAILURE);
        }
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
                    // Allocate memory for the task
                    thread_task_t* task = (thread_task_t*)malloc(sizeof(thread_task_t));
                    if (!task) {
                        perror("malloc failed");
                        continue;
                    }
                    task->sockfd = sockfd;
                    task->client_addr = client_addr;
                    task->addr_len = addr_len;
                    memcpy(task->buffer, buffer, n);
                    task->length = n;

                    // Add task to the queue
                    pthread_mutex_lock(&task_queue_mutex);
                    if (task_queue_size < MAX_EVENTS) {
                        task_queue[task_queue_tail] = task;
                        task_queue_tail = (task_queue_tail + 1) % MAX_EVENTS;
                        task_queue_size++;
                        pthread_cond_signal(&task_queue_cond);
                    } else {
                        fprintf(stderr, "Task queue is full, dropping task\n");
                        free(task);
                    }
                    pthread_mutex_unlock(&task_queue_mutex);
                }
            }
        }
    }

    // Cleanup
    close(epfd);
    close(sockfd);
    return 0;
}

