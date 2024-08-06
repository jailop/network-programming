#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 256

int main(int argc, char *argv[]) {
        char line[BUFSIZE];
        FILE *comm;
        if (argc != 2) {
                fprintf(stderr, "Command client missed\n");
                exit(EXIT_FAILURE);
        }
        comm = popen(argv[1], "w");
        while (1) {
                if (fgets(line, BUFSIZE, stdin) == NULL)
                        break;
                fprintf(comm, "%s\n", line);
        }
        pclose(comm);
        return EXIT_SUCCESS;
}
