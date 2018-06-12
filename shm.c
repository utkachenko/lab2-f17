#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

const uint SHM_SIZE = 64; //ADDED LAB4 (64 pages in shared memory)

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
    int i;
    acquire(&(shm_table.lock));
    for (i = 0; i< SHM_SIZE; i++) {   //looks through reference table to see if segment exsists
        if (shm_table.shm_pages[i].id == id) {   //if it does exsist
            if (mappages(myproc()->pgdir, (char *)PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U) == -1) {
	        release(&(shm_table.lock));
	        return -1;
	    }
            shm_table.shm_pages[i].refcnt += 1;       //case 1
            *pointer = (char *)PGROUNDUP(myproc()->sz);
            myproc()->sz += PGSIZE;      //update size
            release(&(shm_table.lock)); 
            return 0;
        }
    }

    for (i = 0; i< SHM_SIZE; i++) {
        if (shm_table.shm_pages[i].id == 0) {     //CASE 2
            shm_table.shm_pages[i].id = id;		 //and store this information in the shm_table
            if ((shm_table.shm_pages[i].frame = kalloc()) == 0) {    //get physical address
                release(&(shm_table.lock));
                return -1;
            }
            memset(shm_table.shm_pages[i].frame, 0, PGSIZE);     //If it doesn’t then it needs to allocate a page 
            shm_table.shm_pages[i].refcnt = 1;              
	    if (mappages(myproc()->pgdir, (char *)PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U) == -1) { //and map it
	        release(&(shm_table.lock));
	        return -1;
	    }
	    *pointer = (char *)PGROUNDUP(myproc()->sz);   //pointer to virtual address
            myproc()->sz += PGSIZE;        //update size
            release(&(shm_table.lock));
            return 0;
        }
    }
    //release the lock but shouldn't get here
    release(&(shm_table.lock));
    return 0;
}

//ADDED LAB 4
int shm_close(int id) {
//you write this too!
  int i;
  acquire(&(shm_table.lock));
  for (i = 0; i < SHM_SIZE; i++) {
    if (shm_table.shm_pages[i].id == id) {
      shm_table.shm_pages[i].refcnt -= 1;
      if (shm_table.shm_pages[i].refcnt <= 0) {
        shm_table.shm_pages[i].id = 0;
        shm_table.shm_pages[i].frame = 0;
        shm_table.shm_pages[i].refcnt = 0;
      }
      release(&(shm_table.lock));
      return 0;
    }
  }
  release(&(shm_table.lock));
  return -1;
}
