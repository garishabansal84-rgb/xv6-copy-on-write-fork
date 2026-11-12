#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int on;

  if (argc != 2) {
    printf("usage: trace 0|1\n");
    exit(1);
  }

  on = atoi(argv[1]);

  if (on != 0 && on != 1) {
    printf("usage: trace 0|1\n");
    exit(1);
  }

  trace(on);

  if (on)
    printf("Tracing enabled\n");
  else
    printf("Tracing disabled\n");

  exit(0);
}
