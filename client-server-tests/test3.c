#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFSIZE 256

int main(int argc, char *argv[]) {
        int n;
        char output[BUFSIZE + 1];
        int fds[2];
        pipe(fds);
        pid_t pid = fork();
        if (pid == 0) {
                dup2(fds[0], STDIN_FILENO);
                close(fds[0]);
                close(fds[1]);
                char *argv[] = {(char*)"python", NULL};
                execvp(argv[0], argv);
        }
        close(fds[0]);
        while(1) {
                if (fgets(output, BUFSIZE, stdin) == NULL) {
                        break;
                }
                n = strlen(output);
                output[n - 1] = 0;
                dprintf(fds[1], "%s\n", output);
        }
        close(fds[1]);
        return EXIT_SUCCESS;        
}
