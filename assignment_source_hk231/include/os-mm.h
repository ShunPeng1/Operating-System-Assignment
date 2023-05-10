#ifndef OSMM_H
#define OSMM_H

//#define MM_PAGING /* Thuan: Please edit this in os-cfg.h instead of here*/
#define PAGING_MAX_MMSWP 4 /* max number of supported swapped space */
#define PAGING_MAX_SYMTBL_SZ 30  /* the max number of variable allowed in each program */


#include <sys/types.h>
#include <pthread.h>

typedef char BYTE;
typedef uint32_t addr_t;
//typedef unsigned int uint32_t;

/*
* A linked list
*/
struct pgn_t{
   int pgn; // page number (14 bit)
   struct pgn_t *pg_next; 
};

/*
 *  Memory region struct
 *  vm_rg_struct is a linked-list
 */
struct vm_rg_struct {
   unsigned long rg_start;
   unsigned long rg_end;
   int valid;  // Note: this field is only initialized if it belongs to the symbol table, no where else

   struct vm_rg_struct *rg_next; 
};

/*
 *  Memory area struct
 *  vm_area_struct is a linked-list
 */
struct vm_area_struct {
   unsigned long vm_id;
   unsigned long vm_start; 
   unsigned long vm_end; // vm_end is used as at the end of the page

   unsigned long sbrk; // sbrk is used as a end at current using memory, vm_start < sbrk <= vm_end, for not allocate so much memory
/*
 * Derived field
 * unsigned long vm_limit = vm_end - vm_start
 */
   struct mm_struct *vm_mm; // its parent is the mm_struct, where it save the used memory region "symrgtbl"
   struct vm_rg_struct *vm_freerg_list; // a head linked-list of free memory region
   struct vm_area_struct *vm_next; 
};

/* 
 * Memory management struct
 * storing page table directory for entries, and the virtual memory of each variable of the process
 */
struct mm_struct {
   uint32_t *pgd; // the "page table" directory, an array contains all page table entries (pte) format

   struct vm_area_struct *mmap; // the head of virual memory area linked list

   /* Currently we support a fixed number of symbol */
   struct vm_rg_struct symrgtbl[PAGING_MAX_SYMTBL_SZ]; // each variable in a process have its virtual memory, this is a fixed array for variable
   
   /* list of free page */
   struct pgn_t *fifo_using_pgn; //head of using page linked list, 
   int current_max_pgn;
   int num_of_fifo_using_pgn;
};

/*
 * physical FRAME is a linked list, framephy_struct is a node that hold the actual frame id
 */
struct framephy_struct { 
   int fpn; // the actual frame page number id
   struct framephy_struct *fp_next; 

   /* Resereed for tracking allocated framed */
   struct mm_struct* owner;
};

/*
Physical memory struct, using a array of byte to store data, have 2 mode
- randomly: when adding or removing, we create a random address to store/load at that address
- serially: when adding or removing, we traverse from 0 -> maxsz
 */
struct memphy_struct {
   /* Basic field of data and size */
   BYTE *storage; // byte array
   int maxsz; // max size of byte array
   
   /* Sequential device fields */ 
   int rdmflg; // the memory access is randomly (rdmflg = 1) or serially access(rdmflg = 0)
   int cursor; // for iterating during serially add

   /* Management structure */
   struct framephy_struct *free_fp_list; // free frame linked list
   int num_of_free_frame;

   struct framephy_struct *used_fp_list; // used frame linked list
   int num_of_used_frame;

   /* Mutex lock for mutiple cpu access*/
#if SYNC_MM
   pthread_mutex_t lock_free_fp; // lock for free frame list
   pthread_mutex_t lock_used_fp; // lock for used frame list
   pthread_mutex_t lock_storage; // lock for storage
#endif // SYNC_SCHED
};

#endif
