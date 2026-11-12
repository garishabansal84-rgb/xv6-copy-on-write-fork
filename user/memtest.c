
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    int pid;
    int fds[2];
    char *p;
    uint64 before, after_fork, after_write;
    int pages = 100;
    char signal = 'X';

    p = sbrk(pages * 4096);

    if (p == (char *)-1) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    for (int i = 0; i < pages; i++)
        p[i * 4096] = 'A';

    before = freemem();

    pipe(fds);

    pid = fork();

    if (pid < 0) {
        printf("Fork failed\n");
        exit(1);
    }

    if (pid == 0) {
        close(fds[0]);

        for (int i = 0; i < pages; i++)
            p[i * 4096] = 'B';

        write(fds[1], &signal, 1);
        close(fds[1]);

        printf("Child finished writing.\n");
        
        exit(0);
    }

    close(fds[1]);

    after_fork = freemem();

    read(fds[0], &signal, 1);

    after_write = freemem();

    wait(0);

    printf("Free memory before fork: %d bytes\n", (int)before);
    printf("Free memory after fork: %d bytes\n", (int)after_fork);
    printf("Free memory after child writes: %d bytes\n", (int)after_write);

    exit(0);
}
