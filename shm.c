#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
	acquire(&shm_table.lock);

	struct proc *curproc = myproc();
	pde_t *pgdir = curproc->pgdir;
	unit va = PGROUNDUP(curproc->sz) //essentially gets the next available page-aligned address
	
	//find shared memory segment in the shm_table
	for (int i = 0; i < 64; i++) {
		if (shm_table.shm_pages[i].id == id) {
			//implementing Case1: segment exists
			shm_table.shm_pages[i].refcnt++;
			mappages(pgdir, (void *)va, PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W | PTE_U); 
			*pointer = (char *)va;
			curproc->sz = va + PGSIZE;
			release(&shm_table.lock);
			return 0; 
		}
	}


	//implementing case2: segment doesn't exist
	for (int i = 0; i < 64; i++) {
		if (shm_table.shm_pages[i].id == 0) {
			shm_table.shm_pages[i].id = id; 
			shm_table.shm_pages[i].frame = (char *)kmalloc(); 
			if (shm_table.shm_pages[i].frame == 0) {
				release(&shm_table.lock); 
				return -1; //kmalloc has failed
			}
			shm_table.shm_pages[i].refcnt = 1;
			mappages(pgdir, (void *)va, PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W | PTE_U);
			*pointer = (char *)va;
			curproc->sz = va + PGSIZE;
			release(&shm_table.lock);
			return 0;
		}
	}

	release(&shm_table.lock);
	return -1; //no available entries in shm_table
}


int shm_close(int id) {
//you write this too!




return 0; //added to remove compiler warning -- you should decide what to return
}
