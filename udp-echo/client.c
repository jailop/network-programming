// Simplyfied DHCP client

#include <stdio.h>       // fprintf, printf
#include <stdlib.h>      // atoi
#include <string.h>      // memset
#include <unistd.h>      // close
#include <sys/socket.h>  // socket
#include <arpa/inet.h>   // htons

#define BUFSIZE 2048

void show_help(const char *progname) {
    printf(
        "Usage:\n"
        "    %s PORT\n"
        "Where:\n"
        "    PORT     : port in which server will be listening\n",
        progname
    );
}

int request(int port) {
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
    socklen_t len = sizeof(server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    char buffer[BUFSIZE];
    strcpy(buffer, "Test message");

    int n = sendto(sockfd, buffer, strlen(buffer), MSG_CONFIRM, (struct sockaddr *) &server_addr, len);
    if (-1 == n) {
        perror("Error: sending response");
        close(sockfd);
        return -1;
    }

    n = recvfrom(sockfd, buffer, BUFSIZE - 1, MSG_WAITALL, (struct sockaddr *) &server_addr, &len);
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
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: invalid number of arguments\n");
        show_help(argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);
    if (0 == port) {
        fprintf(stderr, "Error: invlaid port number");
        show_help(argv[0]);
    }

    if (-1 == request(port)) {
        fprintf(stderr, "Error: request cannot be processed\n");
        return -1;
    }

    return 0;
}
