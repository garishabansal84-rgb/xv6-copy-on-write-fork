
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
    char *p;
    int pid;
    int fds[2];
    int faults, copies;

    printf("Starting COW tests...\n");

    p = sbrk(4096);
    if (p == (char *)-1) {
        printf("FAIL: sbrk\n");
        exit(1);
    }

    p[0] = 'A';

    pid = fork();

    if (pid < 0) {
        printf("FAIL: fork\n");
        exit(1);
    }

    if (pid == 0) {
        p[0] = 'B';

        if (cowstats(&faults, &copies) < 0) {
            printf("cowstats failed\n");
            exit(1);
        }

        printf("Child COW faults: %d\n", faults);
        printf("Child page copies: %d\n", copies);

        if (p[0] != 'B') {
            printf("FAIL: child write\n");
            exit(1);
        }

        exit(0);
    }

    wait(0);

    if (p[0] != 'A') {
        printf("FAIL: parent memory changed\n");
        exit(1);
    }

    printf("Test 1: Parent-child isolation OK\n");

    p[0] = 'C';

    if (p[0] != 'C') {
        printf("FAIL: parent write\n");
        exit(1);
    }

    printf("Test 2: Parent write OK\n");

    if (pipe(fds) < 0) {
        printf("FAIL: pipe\n");
        exit(1);
    }

    p[0] = 'A';

    pid = fork();

    if (pid < 0) {
        printf("FAIL: fork\n");
        exit(1);
    }

    if (pid == 0) {
        close(fds[1]);

        if (read(fds[0], p, 1) != 1) {
            printf("FAIL: child read\n");
            exit(1);
        }

        if (p[0] != 'X') {
            printf("FAIL: copyout data\n");
            exit(1);
        }

        close(fds[0]);
        exit(0);
    }

    close(fds[0]);

    if (write(fds[1], "X", 1) != 1) {
        printf("FAIL: pipe write\n");
        exit(1);
    }

    close(fds[1]);
    wait(0);

    if (p[0] != 'A') {
        printf("FAIL: copyout changed parent memory\n");
        exit(1);
    }

    printf("Test 3: COW copyout OK\n");

    if (cowstats(&faults, &copies) < 0) {
        printf("cowstats failed\n");
        exit(1);
    }

    printf("Parent COW faults: %d\n", faults);
    printf("Parent page copies: %d\n", copies);

    printf("ALL COW TESTS PASSED\n");

    exit(0);
}
