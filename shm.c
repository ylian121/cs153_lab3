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

//you write this




return 0; //added to remove compiler warning -- you should decide what to return
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

