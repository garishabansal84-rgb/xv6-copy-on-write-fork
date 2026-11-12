#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  struct rtcdate d;

  if(date(&d) < 0){
    printf("date syscall failed\n");
    exit(1);
  }

  printf("%d/%d/%d\n", d.day, d.month, d.year);
  printf("%d:%d:%d\n", d.hour, d.minute, d.second);

  exit(0);
}
