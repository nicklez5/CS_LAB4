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
  int i;
  acquire(&(shm_table.lock));
  for(i = 0; i < 64; i++){

    //Case 1 
    if(shm_table.shm_pages[i].id == id){
       mappages(myproc()->pgdir, (void*)PGROUNDUP(myproc()->sz), PGSIZE , V2P(shm_table.shm_pages[i].frame), PTE_W | PTE_U);
       shm_table.shm_pages[i].refcnt++;
       *pointer = (char *)PGROUNDUP(myproc()->sz); 
       myproc()->sz = PGROUNDUP(myproc()->sz) + PGSIZE;
       //myproc()->sz += PGSIZE;
       //break;
    }
  }
   
  for(i = 0; i < 64; i++){

     //Case 2 
     if(shm_table.shm_pages[i].id == 0){

        shm_table.shm_pages[i].id = id; 
        //ALLOCATING A PAGE 
        shm_table.shm_pages[i].frame = kalloc();

        //ADD 0's TO THAT PAGE FRAME
        memset(shm_table.shm_pages[i].frame,0, PGSIZE);
        shm_table.shm_pages[i].refcnt = 1;

        mappages(myproc()->pgdir, (void *)PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W | PTE_U);
        *pointer = (char *)PGROUNDUP(myproc()->sz);
        myproc()->sz = PGROUNDUP(myproc()->sz) + PGSIZE;
    }
  }
 
   
  release(&(shm_table.lock));
  return 0; 
}


int shm_close(int id){
   acquire(&(shm_table.lock));
   int i;
   for(i = 0; i < 64; i++){
      if(shm_table.shm_pages[i].id == id){
        if(shm_table.shm_pages[i].refcnt > 1){
            shm_table.shm_pages[i].refcnt--;
        }else{
            shm_table.shm_pages[i].id = 0;
            shm_table.shm_pages[i].frame = 0;
            shm_table.shm_pages[i].refcnt = 0;
        }
        
      }
   }
   release(&(shm_table.lock));
   return 0; 

   //added to remove compiler warning -- you should decide what to return
}
