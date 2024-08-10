#include <stdio.h>
#include <stdlib.h>
#include <string.h>      // memset
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>   // htons
#include <pthread.h>

#define PORT 9999
#define SRVADDR "0.0.0.0"
#define BUFSIZE 1024

// Variables to record number of requests and
// responses processed by unit of time.
_Atomic unsigned long atomic_request_count = 0;
_Atomic unsigned long atomic_response_count = 0;
_Atomic int keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

void *report(void *arg) {
    while (keep_running) {
        sleep(1);
        printf("%lu %lu\n", atomic_request_count, atomic_response_count);
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
        while ((n_recv = recv(cltsock, buffer, BUFSIZE, 0)) > 0) {
            atomic_request_count++;
            // Echoing data
            ssize_t n_send = send(cltsock, buffer, BUFSIZE, 0);
            if (n_send == -1) {
                perror("send");
                break;
            }
            atomic_response_count++;
        }
        if (n_recv == -1) {
            perror("recv");
        }
}

void serve(int srvsock) {
    while (keep_running) {
        // Accepting connections
        struct sockaddr_in cltaddr;
        socklen_t cltlen = sizeof(struct sockaddr_in);
        int cltsock = accept(srvsock, (struct sockaddr *) &cltaddr, &cltlen);
        if (cltsock == -1) {
            perror("connect");
            continue;
        }
        char *client_address = inet_ntoa(((struct sockaddr_in *) &cltaddr)->sin_addr);
        printf("Connection accepted from %s\n", client_address);
        process_request(cltsock);
        close(cltsock);
    }
}

int main(void) {
    int res;  // To checked result for system functions
    signal(SIGINT, signal_handler);
    // Openning a socket
    int srvsock = socket(AF_INET, SOCK_STREAM, 0);
    if (srvsock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
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
    // Launching reporter thread
    pthread_t report_id;
    res = pthread_create(&report_id, NULL, report, NULL);
    if (res != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    // Start serving
    printf("Server is running on port %d\n", PORT);
    puts("Press Ctrl+C to shutdown the server\n");
    serve(srvsock);
    // Cleanning
    puts("Shutting down...\n");
    pthread_join(report_id, NULL);
    close(srvsock);
    return EXIT_SUCCESS;
}
