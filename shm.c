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
  struct proc *curproc = myproc(); 
  int i, page_num, yy;
  page_num = -1; 
  int this_sz;
  acquire(&(shm_table.lock));
  for(i = 0; i < 64; i++){
    if(shm_table.shm_pages[i].id == id){
       page_num = i;
    }
  }


  //It already exists 
  /* Increase ref count, MAPPAGES VA & PA
   * UPDATE SZ */  
  if(page_num != -1){
    this_sz = PGROUNDUP(curproc->sz);
    mappages(curproc->pgdir, (void*)this_sz, PGSIZE, V2P(shm_table.shm_pages[page_num].frame), PTE_W | PTE_U);
    shm_table.shm_pages[page_num].refcnt++;
    *pointer = (char *)this_sz;
    curproc->sz = this_sz + PGSIZE;
    //*pointer = (char *)curproc->sz; 
     
  }else{
    //It needs to allocate a page, map it and store this information in the shm_table
    for(yy = 0; yy < 64; yy++){
        if(shm_table.shm_pages[yy].id == 0){
            page_num = yy;
        }
    } 
    shm_table.shm_pages[page_num].id = id;
    shm_table.shm_pages[page_num].frame = kalloc();
    shm_table.shm_pages[page_num].refcnt = 1;

    this_sz = PGROUNDUP(curproc->sz); 

    mappages(curproc->pgdir, (void *)this_sz, PGSIZE, V2P(shm_table.shm_pages[page_num].frame), PTE_W | PTE_U);
    *pointer = (char *)this_sz;
    curproc->sz = this_sz + PGSIZE;	
  }
  release(&(shm_table.lock)); 
    



  return 0; 
}


int shm_close(int id){

   int i, page_num;
   page_num = -1;
   
   acquire(&(shm_table.lock));
   for(i = 0; i < 64; i++){
      if(shm_table.shm_pages[i].id == id){
         page_num = i;
      }
   }
   if(page_num != -1){
      //acquire(&(shm_table.lock));
      shm_table.shm_pages[page_num].refcnt -= 1;
      if(shm_table.shm_pages[page_num].refcnt == 0){
         shm_table.shm_pages[page_num].id = 0;
         shm_table.shm_pages[page_num].frame = 0;
         shm_table.shm_pages[page_num].refcnt = 0;
      }
      release(&(shm_table.lock));      
   }else{
        release(&(shm_table.lock));
   }         
   return 0; 
   //added to remove compiler warning -- you should decide what to return
}
