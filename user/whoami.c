#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if(argc != 2){
        printf("Usage: whoami <pid>\n");
        exit(1);
    }

    int pid = atoi(argv[1]);

    int forkcount = getforkcount(pid);

    if(forkcount < 0){
        printf("process not found\n");
        exit(1);
    }

    int pids[10];

    int n = getancestors(pid, pids, 10);

    if(n < 0){
        printf("process not found\n");
        exit(1);
    }

    printf("pid %d, forkcount %d, ancestors:", pid, forkcount);

    for(int i = 0; i < n; i++){
        printf(" %d", pids[i]);
    }

    printf("\n");

    exit(0);
}
