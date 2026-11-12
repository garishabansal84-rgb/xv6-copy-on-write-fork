#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "printf.h"
#include "vm.h"
#define RTC_TIME_LOW   0x00
#define RTC_TIME_HIGH  0x04
#define MAX_HISTORY 20
#define MAX_CMD 100
extern struct spinlock wait_lock;
extern struct proc *initproc;
struct history_entry {
  char command[MAX_CMD];
};

struct history_entry history_buffer[MAX_HISTORY];
int history_count = 0;
void
add_history(char *cmd)
{
  int i;

  if(history_count < MAX_HISTORY){
    for(i = 0; i < MAX_CMD - 1 && cmd[i] != '\0'; i++)
      history_buffer[history_count].command[i] = cmd[i];

    history_buffer[history_count].command[i] = '\0';

    history_count++;
  }
  else{
    for(i = 1; i < MAX_HISTORY; i++)
      history_buffer[i - 1] = history_buffer[i];

    for(i = 0; i < MAX_CMD - 1 && cmd[i] != '\0'; i++)
      history_buffer[MAX_HISTORY - 1].command[i] = cmd[i];

    history_buffer[MAX_HISTORY - 1].command[i] = '\0';
  }
}
uint64
sys_history_add(void)
{
  uint64 addr;
  char cmd[MAX_CMD];

  argaddr(0, &addr);

  if (copyin(myproc()->pagetable,
           myproc()->sz,
           cmd,
           addr,
           MAX_CMD) < 0)
    return -1;

  cmd[MAX_CMD - 1] = '\0';

  add_history(cmd);

  return 0;
}
uint64
sys_history(void)
{
  uint64 addr;
  int i;
  int offset = 0;

  argaddr(0, &addr);

  for(i = 0; i < history_count; i++){
    char line[MAX_CMD];

    // Create: "1  ls\n"
    int n = 0;
    int num = i + 1;

    if(num >= 10){
      line[n++] = '0' + (num / 10);
      line[n++] = '0' + (num % 10);
    }
    else{
      line[n++] = '0' + num;
    }

    line[n++] = ' ';
    line[n++] = ' ';

    int j;
    for(j = 0; j < MAX_CMD - n - 2 &&
              history_buffer[i].command[j] != '\0'; j++)
      line[n++] = history_buffer[i].command[j];

    line[n++] = '\n';
    line[n] = '\0';

    if(copyout(myproc()->pagetable,
               myproc()->sz,
               addr + offset,
               line,
               n) < 0)
      return -1;

    offset += n;
  }

  return offset;
}

uint64
sys_getancestors(void)
{
    int pid;
    uint64 upids;
    int max;

    argint(0, &pid);
    argaddr(1, &upids);
    argint(2, &max);

    if(max <= 0)
        return 0;

    int ancestors[10];
    int count = 0;

    struct proc *p = findproc(pid);

    if(p == 0)
        return -1;

    while(p->parent != 0 && count < max){

        struct proc *parent = p->parent;

        release(&p->lock);

        acquire(&parent->lock);

        ancestors[count++] = parent->pid;

        p = parent;
    }

    release(&p->lock);

    if(copyout(myproc()->pagetable,
               upids,
               0,
               (char *)ancestors,
               count * sizeof(int)) < 0)
        return -1;

    return count;
}

struct rtcdate {
  int second;
  int minute;
  int hour;
  int day;
  int month;
  int year;
};

static uint64
rtc_read(void)
{
  volatile uint32 *low;
  volatile uint32 *high;
  uint32 lo, hi;

  low = (volatile uint32 *)(RTC + RTC_TIME_LOW);
  high = (volatile uint32 *)(RTC + RTC_TIME_HIGH);

  lo = *low;
  hi = *high;

  return ((uint64)hi << 32) | lo;
}

static int
isleap(int year)
{
  return (year % 4 == 0 && year % 100 != 0) ||
         (year % 400 == 0);
}
uint64
sys_getforkcount(void)
{
    int pid;
    argint(0, &pid);

    struct proc *p = findproc(pid);

    if(p == 0)
        return -1;

    int count = p->forkcount;

    release(&p->lock);

    return count;
}

uint64
sys_date(void)
{
  struct rtcdate r;
  uint64 ns;
  uint64 seconds;
  uint64 days;
  int days_in_month[] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
  };

  uint64 addr;

  argaddr(0, &addr);

  /*
   * RTC gives nanoseconds since January 1, 1970 UTC.
   */
  ns = rtc_read();
  seconds = ns / 1000000000ULL;
  seconds += 19800; 
  r.second = seconds % 60;
  seconds /= 60;

  r.minute = seconds % 60;
  seconds /= 60;

  r.hour = seconds % 24;
  days = seconds / 24;

  /*
   * Convert number of days since 1970-01-01
   * into year/month/day.
   */
  r.year = 1970;

  while (days >= (uint64)(isleap(r.year) ? 366 : 365)) {
    days -= isleap(r.year) ? 366 : 365;
    r.year++;
  }

  r.month = 1;

  if (isleap(r.year))
    days_in_month[1] = 29;

  while (days >= (uint64)days_in_month[r.month - 1]) {
    days -= days_in_month[r.month - 1];
    r.month++;
  }

  r.day = days + 1;

  if (copyout(myproc()->pagetable,
            myproc()->sz,
            addr,
            (char *)&r,
            sizeof(r)) < 0)
  return -1;

  return 0;
}
uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
extern int tracing_enabled;

uint64
sys_trace(void)
{
  int on;

  argint(0, &on);
  tracing_enabled = on;

  return 0;
}

uint64
sys_cowstats(void)
{
    uint64 fault_addr;
    uint64 copy_addr;
    struct proc *p = myproc();

    int faults = p->cow_faults;
    int copies = p->cow_copies;

    argaddr(0, &fault_addr);
    argaddr(1, &copy_addr);

    if(copyout(p->pagetable, p->sz, fault_addr,
               (char *)&faults, sizeof(int)) < 0)
        return -1;

    if(copyout(p->pagetable, p->sz, copy_addr,
               (char *)&copies, sizeof(int)) < 0)
        return -1;

    return 0;
}
uint64
sys_freemem(void)
{
    return getfreemem();
}