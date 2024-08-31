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

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

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
	uint va = PGROUNDUP(curproc->sz); //essentially gets the next available page-aligned address
	
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
			shm_table.shm_pages[i].frame = (char *)kalloc(); 
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
  int matchingID = 0;
  int i = 0; 
  struct proc* currentProcess = myproc();
  acquire(&(shm_table.lock)); 
  for(i = 0; i < 64; i++){ 
    if(shm_table.shm_pages[i].id == id){ 
      shm_table.shm_pages[i].refcnt--;
      matchingID = 1; 
      if(shm_table.shm_pages[i].refcnt > 0){
        break;
      }
      else{
        uint va;
        for (va = 0; va < currentProcess->sz; va += PGSIZE) {
          pte_t *pte = walkpgdir(currentProcess->pgdir, (void*)va, 0); 
          lcr3(V2P(currentProcess->pgdir));
          if (pte && (*pte & PTE_P) && PTE_ADDR(*pte) == V2P(shm_table.shm_pages[i].frame)) {
            *pte = 0;
            lcr3(V2P(currentProcess->pgdir));
            break;
          }
        }
        shm_table.shm_pages[i].frame = 0;
        shm_table.shm_pages[i].id = 0;

        break;
      }
    }
  }
  release(&(shm_table.lock));
  return matchingID == 0 ? 1 : 0;
}

