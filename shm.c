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
    shm_table.shm_pages[i].id = 0;
    shm_table.shm_pages[i].frame = 0;
    shm_table.shm_pages[i].refcnt = 0;
  }
  release(&(shm_table.lock));
}

//ID : Shared memory segment
//Pointer: USED to return a pointer to the shared page | RETURN VIRTUAL ADDRESS ON THIS CALL
int shm_open(int id, char **pointer) { 

  acquire(&(shm_table.lock));

  int i, page_not_found;
  page_not_found = 0;
  struct proc *curproc = myproc();
  
  //Check if id exists
  for(i = 0; i < 64; i++){
    if(shm_table.shm_pages[i].id == id){
        break;
    }
    if(shm_table.shm_pages[i].id == 0 && page_not_found == 0)
        page_not_found = i;
  }
  //ID does not exists
  if(i >= 64 && page_not_found != 0){
    i = page_not_found;

    shm_table.shm_pages[i].frame = kalloc();
    memset(shm_table.shm_pages[i].frame, 0, PGSIZE);
    shm_table.shm_pages[i].id = id; 
    shm_table.shm_pages[i].refcnt = 1;
    
    uint va = PGROUNDUP(curproc->sz);
    mappages(curproc->pgdir, (void *)va, PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W | PTE_U);

    *pointer = (char *)va;
    curproc->sz = PGROUNDUP(curproc->sz) + PGSIZE;

  //ID does exist
  }else if (i < 64){

    uint va = PGROUNDUP(curproc->sz);
    mappages(curproc->pgdir, (void *)va, PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W | PTE_U);
    shm_table.shm_pages[i].refcnt++;
    *pointer = (char *)va;
    curproc->sz = PGROUNDUP(curproc->sz) + PGSIZE;     

  }else{

  }
    
    release(&(shm_table.lock));
    return 0; 
}


int shm_close(int id){
   int i;
   acquire(&(shm_table.lock));
   for(i = 0; i < 64; i++){
      if(shm_table.shm_pages[i].id == id)
        break;
   }
   if(i < 64){
        shm_table.shm_pages[i].refcnt--;
        if(shm_table.shm_pages[i].refcnt == 0){
            shm_table.shm_pages[i].frame = 0;
            shm_table.shm_pages[i].id = 0;
        }
   }

   release(&(shm_table.lock));
   return 0; 

   //added to remove compiler warning -- you should decide what to return
}
