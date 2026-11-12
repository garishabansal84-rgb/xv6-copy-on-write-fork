
// Physical memory allocator with reference counting for COW.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// One reference count for each physical page.
int ref_count[PHYSTOP / PGSIZE];

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;

  p = (char *)PGROUNDUP((uint64)pa_start);

  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE) {
    // Initially mark the page as having one reference.
    acquire(&kmem.lock);
    ref_count[(uint64)p / PGSIZE] = 1;
    release(&kmem.lock);

    kfree(p);
  }
}

void
kfree(void *pa)
{
  struct run *r;
  uint64 index = (uint64)pa / PGSIZE;

  if (((uint64)pa % PGSIZE) != 0 ||
      (char *)pa < end ||
      (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&kmem.lock);

  if (ref_count[index] < 1)
    panic("kfree ref_count");

  ref_count[index]--;

  // Other processes are still using this page.
  if (ref_count[index] > 0) {
    release(&kmem.lock);
    return;
  }

  release(&kmem.lock);

  // No references remain. We can free the page.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

void *
kalloc(void)
{
  struct run *r;
  uint64 index;

  acquire(&kmem.lock);

  r = kmem.freelist;

  if (r) {
    kmem.freelist = r->next;

    index = (uint64)r / PGSIZE;
    ref_count[index] = 1;
  }

  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE);

  return (void *)r;
}

// Increase the reference count when sharing a physical page.
void
krefinc(uint64 pa)
{
  if ((pa % PGSIZE) != 0 || pa >= PHYSTOP)
    panic("krefinc");

  acquire(&kmem.lock);
  ref_count[pa / PGSIZE]++;
  release(&kmem.lock);
}

// Return the current reference count of a physical page.
int
krefcount(uint64 pa)
{
  int count;

  if ((pa % PGSIZE) != 0 || pa >= PHYSTOP)
    panic("krefcount");

  acquire(&kmem.lock);
  count = ref_count[pa / PGSIZE];
  release(&kmem.lock);

  return count;
}
uint64
getfreemem(void)
{
    struct run *r;
    uint64 count = 0;

    acquire(&kmem.lock);

    r = kmem.freelist;
    while (r) {
        count++;
        r = r->next;
    }

    release(&kmem.lock);

    return count * PGSIZE;
}