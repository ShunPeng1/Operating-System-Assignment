#ifndef OSMM_H
#define OSMM_H

//#define MM_PAGING /* Thuan: Please edit this in os-cfg.h instead of here*/
#define PAGING_MAX_MMSWP 4 /* max number of supported swapped space */
#define PAGING_MAX_SYMTBL_SZ 30  /* the max number of variable allowed in each program */

typedef char BYTE;
typedef uint32_t addr_t;
//typedef unsigned int uint32_t;

/*
* Page
* A linked list
*/
struct pgn_t{
   int pgn;
   struct pgn_t *pg_next; 
};

/*
 *  Memory region struct
 *  vm_rg_struct is a linked-list
 */
struct vm_rg_struct {
   unsigned long rg_start;
   unsigned long rg_end;

   struct vm_rg_struct *rg_next; 
};

/*
 *  Memory area struct
 *  vm_area_struct is a linked-list
 */
struct vm_area_struct {
   unsigned long vm_id;
   unsigned long vm_start; 
   unsigned long vm_end;

   unsigned long sbrk; // sbrk is a limit, vm_start < sbrk <= vm_end, for not allocate so much memory
/*
 * Derived field
 * unsigned long vm_limit = vm_end - vm_start
 */
   struct mm_struct *vm_mm; // its parent is the mm_struct, where it save the used memory region "symrgtbl"
   struct vm_rg_struct *vm_freerg_list; // a linked-list of free memory region
   struct vm_area_struct *vm_next; 
};

/* 
 * Memory management struct
 * storing page table directory for entries, and the virtual memory of each variable of the process
 */
struct mm_struct {
   uint32_t *pgd; // the page table directory, contains all page table entries

   struct vm_area_struct *mmap; // the head of virual memory area linked list

   /* Currently we support a fixed number of symbol */
   struct vm_rg_struct symrgtbl[PAGING_MAX_SYMTBL_SZ]; // each variable in a process have its virtual memory, this is a fixed array for variable
   
   /* list of free page */
   struct pgn_t *fifo_pgn; //head of free page linked list, 
};

/*
 * FRAME/MEM PHY struct
 */
struct framephy_struct { 
   int fpn;
   struct framephy_struct *fp_next;

   /* Resereed for tracking allocated framed */
   struct mm_struct* owner;
};

struct memphy_struct {
   /* Basic field of data and size */
   BYTE *storage;
   int maxsz;
   
   /* Sequential device fields */ 
   int rdmflg; // the memory access is randomly (rdmflg = 1) or serially access(rdmflg = 0)
   int cursor;

   /* Management structure */
   struct framephy_struct *free_fp_list; // free frame list
   struct framephy_struct *used_fp_list; // used frame list
};

#endif
