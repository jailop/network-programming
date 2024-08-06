#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <zmq.h>

#define BUFSIZE 256
#define TRUE 1

int main() {
        void *context, *requester;
        int rc;
        if ((context = zmq_ctx_new()) == NULL) {
                perror("ZMQ context creation failed");
                exit(EXIT_FAILURE);
        }
        if ((requester = zmq_socket(context, ZMQ_REQ)) == NULL) {
                perror("ZMQ socket creation failed");
                exit(EXIT_FAILURE);
        }
        if ((rc = zmq_connect(requester, "tcp://localhost:5555")) != 0) {
                perror("ZMQ socket connection failed");
                exit(EXIT_FAILURE);
        }
        while (TRUE) {
                int n;
                char buffer[BUFSIZE + 1];
                if (fgets(buffer, BUFSIZE, stdin) == NULL)
                        break;
                n = strlen(buffer);
                if (zmq_send(requester, buffer, n - 1, 0) == -1) {
                        perror("ZMQ send message failed");
                        continue;
                }
                n = zmq_recv(requester, buffer, BUFSIZE, 0);
                n = n < BUFSIZE ? n : BUFSIZE;
                buffer[n] = 0;
                printf("%s\n", buffer);
        }
        return EXIT_SUCCESS;
}
