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
} kmem;

#ifdef LAB_PGTBL
// Superpage allocator for large allocations.
struct {
  struct spinlock lock;
  struct run *freelist;
  uint64 start;
  uint64 end;
} supermem;

#define NSUPERPAGE 16
static char superpages[NSUPERPAGE * SUPERPGSIZE] __attribute__((aligned(SUPERPGSIZE)));
static int superpage_initialized = 0;
#endif

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);

#ifdef LAB_PGTBL
  initlock(&supermem.lock, "supermem");
  supermem.freelist = 0;
  supermem.start = (uint64)superpages;
  supermem.end = supermem.start + sizeof(superpages);

  // 初始化超级页面空闲列表
  for(int i = 0; i < NSUPERPAGE; i++) {
    superfree((void*)(supermem.start + i * SUPERPGSIZE));
  }
  superpage_initialized = 1;
#endif
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
#ifdef LAB_PGTBL
    // 避免释放超级页面预留区域
    if((uint64)p >= supermem.start && (uint64)p < supermem.end)
      continue;
#endif
    kfree(p);
  }
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

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

#ifdef LAB_PGTBL
// 分配一个超级页面 (2MB)
void *
superalloc(void)
{
  struct run *r;

  if(!superpage_initialized)
    return 0;

  acquire(&supermem.lock);
  r = supermem.freelist;
  if(r)
    supermem.freelist = r->next;
  release(&supermem.lock);

  if(r)
    memset((char*)r, 5, SUPERPGSIZE); // fill with junk
  return (void*)r;
}

// 释放一个超级页面
void
superfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % SUPERPGSIZE) != 0 || 
     (uint64)pa < supermem.start || 
     (uint64)pa >= supermem.end)
    panic("superfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, SUPERPGSIZE);

  r = (struct run*)pa;

  acquire(&supermem.lock);
  r->next = supermem.freelist;
  supermem.freelist = r;
  release(&supermem.lock);
}

// 检查地址是否在超级页面区域内
int
is_superpage(uint64 pa)
{
  return pa >= supermem.start && pa < supermem.end;
}
#endif