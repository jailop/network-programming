#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <zmq.h>

#define BUFSIZE 256
#define TRUE 1

int main() {
        void *context, *responder;
        int rc;
        if ((context = zmq_ctx_new()) == NULL) {
                perror("ZMQ context creation failed");
                exit(EXIT_FAILURE);
        }
        if ((responder = zmq_socket(context, ZMQ_REP)) == NULL) {
                perror("ZMQ socket creation failed");
                exit(EXIT_FAILURE);
        }
        if ((rc = zmq_bind(responder, "tcp://*:5555")) != 0) {
                perror("ZMQ socket binding failed");
                exit(EXIT_FAILURE);
        }
        while (TRUE) {
                int n;
                char buffer[BUFSIZE + 1];
                n = zmq_recv(responder, buffer, BUFSIZE, 0);
                n = n < BUFSIZE ? n : BUFSIZE;
                buffer[n] = 0;
                printf("%s\n", buffer);
                for (int i = 0; i < n / 2; i++) {
                        char c = buffer[i];
                        buffer[i] = buffer[n - i - 1];
                        buffer[n - i - 1] = c;
                }
                if (zmq_send(responder, buffer, n, 0) == -1)
                        perror("ZMQ send message failed");
        }
        return EXIT_SUCCESS;
}
