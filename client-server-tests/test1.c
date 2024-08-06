#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 256

void client(int fdin, int fdout) {
        int n;
        char input[BUFSIZE + 1];
        char output[BUFSIZE + 1];
        while (1) {
                if (fgets(output, BUFSIZE, stdin) == NULL) {
                        write(fdout, "QUIT", 4);
                        break;
                }
                n = strlen(output);
                output[n - 1] = 0;
                write(fdout, output, strlen(output));
                n = read(fdin, input, BUFSIZE);
                input[n] = 0;
                printf("%s\n", input);
        }
}

void server(int fdin, int fdout) {
        int i;
        int n;
        char input[BUFSIZE + 1];
        strcpy(input, "START");
        while (1) {
                n = read(fdin, input, BUFSIZE);
                input[n] = 0;
                if (strcmp(input, "QUIT") == 0) {
                        write(fdout, input, strlen(input));
                        break;
                }
                for (i = 0; i < n / 2; i++) {
                        char aux = input[i];
                        input[i] = input[n - i - 1];
                        input[n - i - 1] = aux;
                }
                write(fdout, input, strlen(input));
        }
}

int main(int argc, char *argv[]) {
        // int ret;
        int pid;
        int fda[2], fdb[2];
        /*
        if (setenv("PYTHON_OUTPUT", "/tmp/myjupyter.txt", 1) != 0) {
                perror("Error in environment setting");
                exit(EXIT_FAILURE);
        }
        */
        if (pipe(fda) != 0) {
                perror("Pipe A cannot be created");
                exit(EXIT_FAILURE);
        }
        if (pipe(fdb) != 0) {
                perror("Pipe B cannot be created");
                exit(EXIT_FAILURE);
        }
        pid = fork();
        if (pid == 0) {
                close(fda[0]);
                close(fdb[1]);
                client(fdb[0], fda[1]);
       }
        else {
                close(fda[1]);
                close(fdb[0]);
                server(fda[0], fdb[1]);
        }
        /*
        ret = execl("/usr/bin/python", "", NULL);
        printf("%d\n", ret);
        return ret;
        */
        return 0;
}

