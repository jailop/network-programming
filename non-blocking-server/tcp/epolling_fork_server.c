#include <asm-generic/socket.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>      // memset
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>   // htons
#include <sys/epoll.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

#define PORT 9999
#define SRVADDR "0.0.0.0"
#define BUFSIZE 1024
#define MAXEVENTS 32
#define NUMWORKERS 6

struct worker_info {
    int epfd;
    int sockfd;
} WorkerInfo;

// Variables to record number of requests and
// responses processed by unit of time.
_Atomic unsigned long atomic_request_count = 0;
_Atomic unsigned long atomic_response_count = 0;
_Atomic int atomic_keep_running = 1;

void signal_handler(int sig) {
    atomic_keep_running = 0;
}

void *report(void *arg) {
    while (atomic_keep_running) {
        sleep(1);
        printf("%6d: recv %lu  sent %lu\n", getpid(), atomic_request_count, atomic_response_count);
        atomic_request_count = 0;
        atomic_response_count = 0;
    }
    pthread_exit(NULL);
}

void set_server_address(struct sockaddr_in *srvaddr) {
    memset(srvaddr, '\0', sizeof(struct sockaddr_in));
    srvaddr->sin_family = AF_INET;
    srvaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    // inet_pton(AF_INET, SRVADDR, &(srvaddr->sin_addr));
    srvaddr->sin_port = htons(PORT);
}

void process_request(int cltsock) {
    // Receiving data
    char buffer[BUFSIZE];
    ssize_t n_recv;
    while (atomic_keep_running) {
        n_recv = recv(cltsock, buffer, BUFSIZE, MSG_DONTWAIT);
        if (n_recv > 0) {
            // Echoing data
            ssize_t n_send = send(cltsock, buffer, BUFSIZE, MSG_DONTWAIT);
            if (n_send != -1) {
                // perror("send");
                atomic_response_count++;
            }
        }
        if (n_recv == 0) {
            close(cltsock);
            break;
        }
        if (n_recv == -1) {
            // perror("recv");
            break;
        }
        atomic_request_count++;
    }
}

int accept_connection(int srvsock) {
    struct sockaddr_in cltaddr;
    socklen_t cltlen = sizeof(struct sockaddr_in);
    int cltsock = accept4(srvsock, (struct sockaddr *) &cltaddr, &cltlen, SOCK_NONBLOCK);
    if (cltsock == -1) {
        // perror("connect");
        return -1;
    }
    char *client_address = inet_ntoa(((struct sockaddr_in *) &cltaddr)->sin_addr);
    printf("Connection accepted from %s\n", client_address);
    int flags = fcntl(cltsock, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get");
        close(cltsock);
        return -1;
    } 
    flags = fcntl(cltsock, F_SETFL, flags | O_NONBLOCK);
    if (flags == -1) {
        perror("fcntl set");
        close(cltsock);
        return -1;
    }
    return cltsock;
}

void worker(int epfd, int srvsock) {
    // Launching reporter thread
    pthread_t report_id;
    int res = pthread_create(&report_id, NULL, report, NULL);
    if (res != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    struct epoll_event ev, events[MAXEVENTS];
    while (atomic_keep_running) {
        int nfds = epoll_wait(epfd, events, MAXEVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == srvsock) {
                while (atomic_keep_running) {
                    int cltsock = accept_connection(srvsock);
                    printf("Client socket: %d\n", cltsock);
                    if (cltsock == -1) {
                        if (errno == EAGAIN) break;
                        sleep(1);
                    }
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = cltsock;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, cltsock, &ev);
                }
            }
            else {
                process_request(events[i].data.fd);
                // epoll_ctl(winfo.epfd, EPOLL_CTL_DEL, events[i].data.fd, 0);
            }
        }
    }
}

void serve(int srvsock) {
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1) {
        perror("epoll_create server");
        atomic_keep_running = 0;
    }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = srvsock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, srvsock, &ev);
    for (int i = 0; i < NUMWORKERS; i++)
        if (fork() == 0)
            worker(epfd, srvsock);
    while (wait(NULL) > 0);
    if (epfd != -1) {
        close(epfd);
    }
}

int main(void) {
    int res;  // To checked result for system functions
    signal(SIGINT, signal_handler);
    // Openning a socket
    int srvsock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (srvsock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    res = setsockopt(srvsock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, NULL, 0);
    // Setting server address struct
    struct sockaddr_in srvaddr;
    set_server_address(&srvaddr);
    // Binding to server address
    res = bind(srvsock, (struct sockaddr *) &srvaddr, sizeof(srvaddr));
    if (res == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    res = listen(srvsock, 32);
    if (res == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    // Start serving
    printf("Server is running on port %d\n", PORT);
    puts("Press Ctrl+C to shutdown the server\n");
    serve(srvsock);
    // Cleanning
    puts("Shutting down...\n");
    // pthread_join(report_id, NULL);
    close(srvsock);
    return EXIT_SUCCESS;
}
