// Simplify DHCP Server

#include <stdio.h>       // fprintf, printf
#include <stdlib.h>      // atoi
#include <string.h>      // memset
#include <unistd.h>      // close
#include <signal.h>      // signal
#include <sys/socket.h>  // socket
#include <arpa/inet.h>   // htons

#define BUFSIZE 2048

int serving = 1;

void show_help(const char *progname) {
    printf(
        "Usage:\n"
        "    %s PORT IPADDRESS\n"
        "Where:\n"
        "    PORT     : port in which server will be listening\n"
        "    IPADDRESS: Host IP adreess where server is running\n",
        progname
    );
}

void signal_handler(int sig) {
    printf("\nShutting down...\n");
    serving = 0;
    exit(EXIT_SUCCESS);
}

int process_request(int sockfd) {
    char buffer[BUFSIZE];
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));
    memset(buffer, 0, BUFSIZE);
    
    int n = recvfrom(sockfd, buffer, BUFSIZE - 1, MSG_WAITALL, (struct sockaddr *) &client_addr, &len);
    if (-1 == n) {
        perror("Error: receiving message");
        return -1;
    }
    if (0 == n) {
        perror("Error: message not available");
        return -1;
    }
    buffer[n] = '\0';
    printf("%s\n", buffer);
    
    n = sendto(sockfd, buffer, n, MSG_CONFIRM, (const struct sockaddr *) &client_addr, len);
    if (-1 == n) {
        perror("Error: sending response");
        return -1;
    }
    return 0;
}

int serve(int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sockfd) {
        perror("Error: socket cannot be created");
        return -1;
    }
    int broadcast_enabled = 1;
    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enabled, sizeof(broadcast_enabled))) {
        perror("Error: socket options not applied");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (-1 == bind(sockfd, (const struct sockaddr *) &server_addr, sizeof(server_addr))) {
        perror("Error: binding failed");
        return -1;
    }

    while (serving) {
        process_request(sockfd);
    }
    close(sockfd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: invalid number of arguments\n");
        show_help(argv[0]);
        return -1;
    }

    int port = atoi(argv[2]);
    if (0 == port) {
        fprintf(stderr, "Error: invlaid port number");
        show_help(argv[0]);
    }

    signal(SIGINT, signal_handler);    

    printf("Serving from %s on %d\n", argv[1], port);
    printf("Press Ctrl+C to shutdown this server\n");
    return serve(port);
}
