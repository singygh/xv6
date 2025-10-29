// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define STOP PHYSTOP-28*SUPERPGSIZE

void freerange(void *pa_start, void *pa_end);
void superfreerange(void *pa_start, void *pa_end);


extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  struct run *freelist;
} smem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)STOP);

  // 初始化超级页的空闲列表
  initlock(&smem.lock, "smem");
  superfreerange((void*)STOP, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

void
superfreerange(void *pa_start, void *pa_end)
{
  char *p;
  // 向上对齐到 2MB 边界
  p = (char*)SUPERPGROUNDUP((uint64)pa_start);
  for (; p + SUPERPGSIZE <= (char*)pa_end; p += SUPERPGSIZE) {
    superfree(p);
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

void*
superalloc(void)
{
  struct run *r;

  acquire(&smem.lock);
  r = smem.freelist;
  if (r) {
    smem.freelist = r->next;
  }
  release(&smem.lock);

  if (r) {
    memset(r, 1, SUPERPGSIZE);  // 填垃圾数据，检测悬空引用
  }
  return (void*)r;
}

void
superfree(void *pa)
{
  struct run *r;

  // 检查是否 2MB 对齐、是否在超级页预留区域内
  if (((uint64)pa % SUPERPGSIZE) != 0 || 
      (char*)pa < (char*)STOP || 
      (uint64)pa >= PHYSTOP) {
    panic("superfree");
  }

  memset(pa, 1, SUPERPGSIZE);  // 填垃圾数据

  r = (struct run*)pa;

  acquire(&smem.lock);
  r->next = smem.freelist;
  smem.freelist = r;
  release(&smem.lock);
}
