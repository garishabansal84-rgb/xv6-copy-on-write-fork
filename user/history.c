#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_HISTORY 20
#define MAX_CMD 100

int
main(void)
{
  char buffer[MAX_HISTORY * MAX_CMD + 1];
  int n;

  n = history(buffer);

  if(n < 0){
    printf("history syscall failed\n");
    exit(1);
  }

  buffer[n] = '\0';

  printf("%s", buffer);

  exit(0);
}
