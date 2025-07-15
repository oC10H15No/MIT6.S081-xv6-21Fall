// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void
kinit()
{
  // initlock(&kmem.lock, "kmem");
  for(int i = 0; i < NCPU; i++) {
    char lock_name[8];
    snprintf(lock_name, sizeof(lock_name), "kmem_%d", i);
    initlock(&kmem[i].lock, lock_name);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off(); // disable interrupts to avoid deadlock
  int cpu_id = cpuid();
  pop_off(); // re-enable interrupts


  acquire(&kmem[cpu_id].lock);
  r->next = kmem[cpu_id].freelist;
  kmem[cpu_id].freelist = r;
  release(&kmem[cpu_id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off(); // disable interrupts to avoid deadlock
  int cpu_id = cpuid();
  pop_off(); // re-enable interrupts

  acquire(&kmem[cpu_id].lock);
  r = kmem[cpu_id].freelist;
  if(r)
    kmem[cpu_id].freelist = r->next;
  release(&kmem[cpu_id].lock);

  // if allocation succeeded, fill with junk and return
  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    return (void*)r;
  }

  // If allocation failed, steal a page from another CPU's freelist.
  for(int i = 0; i < NCPU; i++) {
    if(i == cpu_id) // skip the current CPU
      continue;

    acquire(&kmem[i].lock);
    struct run* target_list = kmem[i].freelist;
    if(target_list) {
      r = target_list;
      kmem[i].freelist = r->next;
      release(&kmem[i].lock);
      memset((char*)r, 5, PGSIZE); // fill with junk
      return (void*)r;
    } else {
      release(&kmem[i].lock);
    }
  }

  return 0; // no memory available
}
