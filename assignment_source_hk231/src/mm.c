//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/* 
 * init_pte - Initialize PTE entry
 */
int init_pte(uint32_t *pte,
						 int pre,    // present
						 int fpn,    // FPN
						 int drt,    // dirty
						 int swp,    // swap
						 int swptyp, // swap type
						 int swpoff) //swap offset
{
	if (pre != 0) {
		if (swp == 0) { // Non swap ~ page online
			if (fpn == 0) 
				return -1; // Invalid setting

			/* Valid setting with FPN */
			SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
			CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
			CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

			SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT); 
		} else { // page swapped
			SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
			SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
			CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

			SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT); 
			SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
		}
	}

	return 0;   
}

// Quang modified this
/* 
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(uint32_t *pte, int swptyp, int swpoff)
{
	// SETBIT(*pte, PAGING_PTE_PRESENT_MASK);	// This is very concerning ??? How can a page be present and swapped at the same time ???
	CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);		// Quang changed to this
	SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

	SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
	SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

	return 0;
}

/* 
 * pte_set_swap - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(uint32_t *pte, int fpn)
{
	SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
	CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

	SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT); 

	return 0;
}

/* 
 * vmap_page_range - map a range of page at aligned address
 */
int vmap_page_range(struct pcb_t *caller, // process call
					int addr, // start address which is aligned to pagesz // In ALLOC, this is the vm_area_struct->vm->end
					int pgnum, // num of mapping page
					struct framephy_struct *frames,//head of list of the mapped frames
					struct vm_rg_struct *ret_rg)// return mapped region, the real mapped fp
{                                         // no guarantee all given pages are mapped
	/* TODO map range of frame to address space 
	 *      [addr to addr + pgnum*PAGING_PAGESZ]
	 *      in page table caller->mm->pgd[]
	 */


	int page_number_begin = PAGING_PGN(addr); // the page number at addr 
	int pgit = 0;
	ret_rg->rg_end = ret_rg->rg_start = addr; // at least the very first space is usable	
	
	uint32_t user_define_number = 0; // Thuan: I don't need it yet

	for(struct framephy_struct *roam_node = frames; roam_node != NULL; roam_node = roam_node->fp_next){
		uint32_t frame_number = roam_node->fpn;

		uint32_t pte = 	(1 << 31) | // present
                		(0 << 30) | // swapped
                   		(0 << 29) | // reserved
                  		(0 << 28) | // dirty
                   		(user_define_number << 15) | // user-defined numbering
                   		(frame_number << 0); // frame number
		 		
		caller->mm->pgd[page_number_begin + pgit] = pte; // assign the page table directory [ page number ] = page table entry
		
		/* Enqueue new usage page */
		// Quang
		// enlist_pgn_node(&caller->mm->fifo_pgn, page_number_begin+pgit);
		enlist_tail_pgn_node(&caller->mm->fifo_pgn, page_number_begin+pgit);
		
		pgit++;
	}

	return 0;
}

/* 
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @memphy	  : ram or swap
 * @req_pgnum : request page number
 * @frm_lst   : return the head of the free frame list you created
 * return: the size of the array of frm_list
 */

int alloc_pages_range(struct memphy_struct *memphy , int req_pgnum, struct framephy_struct** frm_lst)
{
	int pgit, fpn;

	for(pgit = 0; pgit < req_pgnum; pgit++)
	{
		if(MEMPHY_get_freefp(memphy, &fpn) == 0) // Successfully Get the free frame number id in fpn
		{
			struct framephy_struct *new_node = malloc(sizeof(struct framephy_struct));

			/* Thuan: Create new node with value fpn, then assign the new node become head of frm_lst */
			new_node->fpn = fpn;
			new_node->fp_next = *frm_lst;
			*frm_lst = new_node;

		} 
		else {  // ERROR CODE of obtaining somes but not enough frames
			/*@bksysnet: author provides a feasible solution of getting frames
			*FATAL logic in here, wrong behaviour if we have not enough page
			*i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
			*Don't try to perform that case in this simple work, it will result
			*in endless procedure of swap-off to get frame and we have not provide 
			*duplicate control mechanism, keep it simple
			*/

			return pgit;
		} 
 	}

	return req_pgnum;
}


/* 
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @increase_page_number  : number of mapped page
 * @ret_rg    : returned region
 */
int vm_map_ram(struct pcb_t *caller, int astart, int aend, int mapstart, int increase_page_number, struct vm_rg_struct *ret_rg)
{
	struct framephy_struct *mram_free_frame_list = NULL;
	int num_increase_alloc_page_by_mram;

	/*@bksysnet: author provides a feasible solution of getting frames
	 *FATAL logic in here, wrong behaviour if we have not enough page
	 *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
	 *Don't try to perform that case in this simple work, it will result
	 *in endless procedure of swap-off to get frame and we have not provide 
	 *duplicate control mechanism, keep it simple
	 */
	num_increase_alloc_page_by_mram = alloc_pages_range(caller->mram, increase_page_number, &mram_free_frame_list);
	//printf("DEBUG: alloc_pages_range\n");
	if (num_increase_alloc_page_by_mram < 0 && num_increase_alloc_page_by_mram != -3000)
		return -1;

	/* Out of memory */
	if (num_increase_alloc_page_by_mram == -3000) 
	{
#ifdef MMDBG
		printf("OOM: vm_map_ram out of memory \n");
#endif
		return -1;
	}

	/* it leaves the case of memory is enough but half in ram, half in swap
	 * do the swaping all to swapper to get the all in ram */
	if(num_increase_alloc_page_by_mram != increase_page_number){ // Thuan TODO swap from swap to main frame
		/*
		struct framephy_struct *mswp_free_frame_list = NULL;
		int increase_page_requesting_by_mswp = increase_page_number - num_increase_alloc_page_by_mram;
		int i_mswp = 0; // from 1 -> 4
		while(increase_page_requesting_by_mswp != 0 && i_mswp < 4){
			increase_page_requesting_by_mswp -= alloc_pages_range(caller->mswp[i_mswp], increase_page_requesting_by_mswp, &mswp_free_frame_list);
			
			i_mswp++;
		}
		// Surely it must have enough size in swap right ?
		*/
	}
	
	vmap_page_range(caller, mapstart, num_increase_alloc_page_by_mram, mram_free_frame_list, ret_rg); // Add the free page and frame to the page table
	//printf("DEBUG: vmap_page_range\n");
	
	MEMPHY_concat_usedfp(caller->mram, mram_free_frame_list); // Add the free frame allocated to the used frame
	//printf("DEBUG: MEMPHY_concat_usedfp\n");
	
	// Update region
	ret_rg->rg_start = astart;
	ret_rg->rg_end = aend;
	ret_rg->rg_next = NULL;
	
	return 0;
}


// Thuan TODO: This page maybe using some where for swap page
/* Swap copy content page from source frame to destination frame 
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, int srcfpn,
								struct memphy_struct *mpdst, int dstfpn) 
{
	int cellidx;
	int physical_address_src,physical_address_dst;
	for(cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
	{
		physical_address_src = srcfpn * PAGING_PAGESZ + cellidx;
		physical_address_dst = dstfpn * PAGING_PAGESZ + cellidx;

		BYTE data;
		MEMPHY_read(mpsrc, physical_address_src, &data);
		MEMPHY_write(mpdst, physical_address_dst, data);
	}

	return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
	struct vm_area_struct * vma = malloc(sizeof(struct vm_area_struct));

	mm->pgd = malloc(PAGING_MAX_PGN*sizeof(uint32_t));

	/* By default the owner comes with at least one vma */
	vma->vm_id = 1;
	vma->vm_start = 0;
	vma->vm_end = vma->vm_start;
	vma->sbrk = vma->vm_start;
	struct vm_rg_struct *first_rg = init_vm_rg(vma->vm_start, vma->vm_end);
	enlist_vm_rg_node(&vma->vm_freerg_list, first_rg);

	vma->vm_next = NULL;
	vma->vm_mm = mm; /*point back to vma owner */

	mm->mmap = vma;
	mm->fifo_pgn = NULL;

	return 0;
}

struct vm_rg_struct* init_vm_rg(int rg_start, int rg_end)
{
	struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

	rgnode->rg_start = rg_start;
	rgnode->rg_end = rg_end;
	rgnode->rg_next = NULL;

	return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct* rgnode)
{
	rgnode->rg_next = *rglist;
	*rglist = rgnode;

	return 0;
}


int enlist_pgn_node(struct pgn_t **plist, int pgn)
{
	struct pgn_t* pnode = malloc(sizeof(struct pgn_t));

	pnode->pgn = pgn;
	pnode->pg_next = *plist;
	*plist = pnode;

	return 0;
}

// Quang modified this to enlist at the tail instead of head to FIFO more easily
int enlist_tail_pgn_node(struct pgn_t **plist, int pgn)
{
	struct pgn_t* pnode = malloc(sizeof(struct pgn_t));
	pnode->pgn = pgn;
	pnode->pg_next = NULL;

	struct pgn_t* head = *plist;
	struct pgn_t **pointer_to_pointer = &head;
	while (*pointer_to_pointer != NULL)
	{
		pointer_to_pointer = &(*pointer_to_pointer)->pg_next;
	}
	*pointer_to_pointer = pnode;

	return 0;
}

// ###################################### Print Debug ##############################
int print_list_fp(struct framephy_struct *ifp)
{
	 struct framephy_struct *fp = ifp;
 
	 printf("print_list_fp: ");
	 if (fp == NULL) {printf("NULL list\n"); return -1;}
	 printf("\n");
	 while (fp != NULL )
	 {
			 printf("fp[%d]\n",fp->fpn);
			 fp = fp->fp_next;
	 }
	 printf("\n");
	 return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
	 struct vm_rg_struct *rg = irg;
 
	 printf("print_list_rg: ");
	 if (rg == NULL) {printf("NULL list\n"); return -1;}
	 printf("\n");
	 while (rg != NULL)
	 {
			 printf("rg[%ld->%ld]\n",rg->rg_start, rg->rg_end);
			 rg = rg->rg_next;
	 }
	 printf("\n");
	 return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
	 struct vm_area_struct *vma = ivma;
 
	 printf("print_list_vma: ");
	 if (vma == NULL) {printf("NULL list\n"); return -1;}
	 printf("\n");
	 while (vma != NULL )
	 {
			 printf("va[%ld->%ld]\n",vma->vm_start, vma->vm_end);
			 vma = vma->vm_next;
	 }
	 printf("\n");
	 return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
	 printf("print_list_pgn: ");
	 if (ip == NULL) {printf("NULL list\n"); return -1;}
	 printf("\n");
	 while (ip != NULL )
	 {
			 printf("va[%d]-\n",ip->pgn);
			 ip = ip->pg_next;
	 }
	 printf("n");
	 return 0;
}

int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
{
	int pgn_start,pgn_end;
	int pgit;

	if(end == -1){
		pgn_start = 0;
		struct vm_area_struct *cur_vma = get_vma_by_index(caller->mm, 0);
		end = cur_vma->vm_end;
	}
	pgn_start = PAGING_PGN(start);
	pgn_end = PAGING_PGN(end);

	printf("print_pgtbl: %d - %d", start, end);
	if (caller == NULL) {printf("NULL caller\n"); return -1;}
		printf("\n");


	for(pgit = pgn_start; pgit < pgn_end; pgit++)
	{
		printf("%08ld: %08x\n", pgit * sizeof(uint32_t), caller->mm->pgd[pgit]);
	}

	return 0;
}

//#endif
