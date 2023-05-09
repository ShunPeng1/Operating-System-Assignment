// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/* Thuan reorganized the code for easy reading*/

// ###################################### ALLOC #########################################

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
	int addr;

	/* By default using vmaid = 0 */
#if PAGING_ERR_DUMP
	int pid = proc->pid;
	int line = proc->pc + 1;
	int alloc_status = __alloc(proc, 0, reg_index, size, &addr);
	if (alloc_status == ERR_AREA_OVERLAP)
	{
		printf("** ShopeeOS ERROR ** in PID: %d, at line: %d:\n ===> what: VMAs overlapped, cannot allocate!\n", pid, line);
	}
	else if (alloc_status == ERR_LACK_MEM_RAM)
	{
		printf("** ShopeeOS ERROR ** in PID: %d, at line: %d:\n ===> what: new region is too large to fit all in RAM, cannot allocate!\n", pid, line);
	}
	else if (alloc_status == ERR_LACK_MEM_SWP)
	{
		printf("** ShopeeOS ERROR ** in PID: %d, at line: %d:\n ===> what: ran out of SWAP memory, cannot allocate!\n", pid, line);
	}
	else if (alloc_status != 0)
	{
		printf("** ShopeeOS ERROR ** in PID: %d, at line: %d:\n ===> what: undefined error, cannot allocate!\n", pid, line);
	}
	return alloc_status;
#else
	return __alloc(proc, 0, reg_index, size, &addr);
#endif
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region (in pgalloc case, it always the first vm area)
 *@rgid: memory region ID (used to identify variable in symbole table) (register index)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 * return 0 if successful, return 1 is unsuccessful
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
	/*Allocate at the toproof */
	struct vm_rg_struct rgnode;

	// printf("DEBUG: ALLOC\n");

	// No TODO in get_free_vmrg_area so ignore
	if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0) // if successfully get free region
	{
		caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
		caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
		caller->mm->symrgtbl[rgid].valid = 1;

		*alloc_addr = rgnode.rg_start;

		return 0;
	}

	// printf("DEBUG: ALLOC NOT FOUND FREE VMRG_AREA\n");
	/* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/

	/*Attempt to increase limit to get space */
	struct vm_area_struct *cur_vma = get_vma_by_index(caller->mm, vmaid);
	// int inc_sz = PAGING_PAGE_ALIGNSZ(size); // new increase size
	// int inc_limit_ret

	if (cur_vma->sbrk + size <= cur_vma->vm_end) // There is enough gap between sbrk and vm_end
	{
		caller->mm->symrgtbl[rgid].rg_start = cur_vma->sbrk;
		caller->mm->symrgtbl[rgid].rg_end = cur_vma->sbrk + size;
		caller->mm->symrgtbl[rgid].valid = 1;

		cur_vma->sbrk += size;
		return 0;
	}

	int old_sbrk = cur_vma->sbrk;

	int demand_size = cur_vma->sbrk + size - cur_vma->vm_end; // 0 + 300 - 0 ; 300 + 100 - 512

	/* TODO INCREASE THE LIMIT */
	int increase_status = increase_vma_limit(caller, vmaid, demand_size, size);
	if (increase_status < 0)
		return increase_status;

	/*Successful increase limit */
	caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
	caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;
	caller->mm->symrgtbl[rgid].valid = 1;
	caller->mm->mmap->sbrk += size;
	cur_vma->vm_end += PAGING_PAGE_ALIGNSZ(demand_size);
	*alloc_addr = old_sbrk;

	/* DEBUGGING */
#if VMDBG
	// print_list_vma(caller->mm->mmap);
	// print_list_rg(&(caller->mm->symrgtbl[vmaid]));
	// print_list_fp(caller->mram->free_fp_list);
	// print_list_pgn(caller->mm->fifo_using_pgn);
	// print_pgtbl(caller, 0 , -1);
#endif // VMDBG
	return 0;
}

/*validate_overlap_vm_area - return -1 if not overlap, return 0 if overlap
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
	/* TODO validate the planned memory area is not overlapped */

	/*Thuan: return -1 if not overlap, return 0 if overlap*/
	// struct vm_area_struct *cur_vma = get_vma_by_index(caller->mm, vmaid);

	// int overlap = cur_vma->vm_start <= vmaend && vmastart <= cur_vma->vm_end; // as long as it overlap
	// int overlap = cur_vma->vm_start <= vmastart && vmaend  <= cur_vma->vm_end; // We need the new create sbrk to be in the start end

	return 0;
}

/*increase_vma_limit - increase vm area limits to reserve space for new variable, return -1 if fail
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@demand_size: additional byte size that we want the os allocate more for us
 *@true_inc_size: the original increase size that we asked for
 */
int increase_vma_limit(struct pcb_t *caller, int vmaid, int demand_size, int true_inc_size)
{
	struct vm_rg_struct *new_region = malloc(sizeof(struct vm_rg_struct));
	int byte_increase_amt = PAGING_PAGE_ALIGNSZ(demand_size); // this is equal to inc_sz
	int num_of_pages_increased = byte_increase_amt / PAGING_PAGESZ;
	struct vm_rg_struct *next_area = create_vm_rg_of_pcb_at_brk(caller, vmaid, true_inc_size); // this is a newly allocated region from [sbrk , sbrk + inc_sz], only a temporary struct to store start and end
	struct vm_area_struct *cur_vma = get_vma_by_index(caller->mm, vmaid);

	int old_end = cur_vma->vm_end;

	/*Validate overlap of obtained region */
	int overlap_status = validate_overlap_vm_area(caller, vmaid, next_area->rg_start, next_area->rg_end);
	if (overlap_status < 0)	   // this check if the new region [sbrk , sbrk + inc_sz] is in [start, end], return -1 if not (meaning sbrk is more than end)
		return overlap_status; /*Overlap and failed allocation */

	/* The obtained vm area (only)
	 * now will be alloc real ram region */
	int map_ram_status = vm_map_ram(caller, next_area->rg_start, next_area->rg_end,
									old_end, num_of_pages_increased, new_region);
	if (map_ram_status < 0) /* Map the memory to MEMRAM */
		return map_ram_status;

	return 0;
}

// ####################################### get vm region ##########################################

/*get_free_vmrg_area - get a free vm region in FREERG_LIST
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *new_region)
{
	struct vm_area_struct *cur_vma = get_vma_by_index(caller->mm, vmaid);

	struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

	if (rgit == NULL)
		return -1;

	/* Probe unintialized new_region */
	new_region->rg_start = new_region->rg_end = -1;

	/* Traverse on list of free vm region to find a fit space */
	while (rgit != NULL)
	{
		if (rgit->rg_start + size <= rgit->rg_end)
		{ /* Current region has enough space */
			new_region->rg_start = rgit->rg_start;
			new_region->rg_end = rgit->rg_start + size;

			/* Update left space in chosen region */
			if (rgit->rg_start + size < rgit->rg_end)
			{
				rgit->rg_start = rgit->rg_start + size;
			}
			else
			{ /*Use up all space (rgit->rg_start + size == rgit->rg_end), remove current node */
				/*Clone next rg node */
				struct vm_rg_struct *nextrg = rgit->rg_next;

				/*Cloning */
				if (nextrg != NULL)
				{
					rgit->rg_start = nextrg->rg_start;
					rgit->rg_end = nextrg->rg_end;

					rgit->rg_next = nextrg->rg_next;

					free(nextrg);
				}
				else
				{								   /*End of free list */
					rgit->rg_start = rgit->rg_end; // dummy, size 0 region
					rgit->rg_next = NULL;		   /*  */
				}
			}
			break;
		}
		else
		{
			rgit = rgit->rg_next; // Traverse next rg
		}
	}

	if (new_region->rg_start == -1) // new region not found
		return -1;

	return 0;
}

/*get_symbol_region_by_id - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symbol_region_by_id(struct mm_struct *mm, int rgid)
{
	if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
		return NULL;

	return &mm->symrgtbl[rgid];
}

/*get_vma_by_index - get vm area by numID, the function traverse the linked list to get the vm_area_struct at the index vmaid
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_index(struct mm_struct *mm, int vmaid)
{
	struct vm_area_struct *pvma = mm->mmap;

	if (mm->mmap == NULL)
		return NULL;

	int vmait = 0;

	while (vmait < vmaid)
	{
		if (pvma == NULL)
			return NULL;

		pvma = pvma->vm_next;
		vmait++;
	}

	return pvma;
}

/*create_vm_rg_of_pcb_at_brk - create vm region of the pcb_t's memory management for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *create_vm_rg_of_pcb_at_brk(struct pcb_t *caller, int vmaid, int size)
{
	struct vm_rg_struct *new_region;
	struct vm_area_struct *cur_vma = get_vma_by_index(caller->mm, vmaid);

	new_region = malloc(sizeof(struct vm_rg_struct));

	new_region->rg_start = cur_vma->sbrk;
	new_region->rg_end = new_region->rg_start + size;

	return new_region;
}

// ########### FREE ###########

// Quang modified this
/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
	struct vm_rg_struct *free_rg_head = mm->mmap->vm_freerg_list;

	if (rg_elmt->rg_start >= rg_elmt->rg_end)
		return -1;

	rg_elmt->rg_next = free_rg_head;

	/* Enlist the new region to the head of linked list*/
	mm->mmap->vm_freerg_list = rg_elmt;
	return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
	// struct vm_rg_struct rgnode;

	if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
		return -1;

	/* TODO: Manage the collect freed region to freerg_list */
	// struct vm_area_struct *cur_vma = get_vma_by_index(caller->mm, vmaid);
	struct vm_rg_struct *removedItem = get_symbol_region_by_id(caller->mm, rgid);
	removedItem->valid = 0;
	// struct vm_rg_struct *delListHead = cur_vma->vm_freerg_list;
	// if(delListHead == NULL){
	// 	delListHead = &removeItem;
	// 	delListHead->rg_start = removeItem->rg_start;
	// 	delListHead->rg_end = removeItem->rg_end;
	// }
	// else{
	// 	while(delListHead->rg_next != NULL){
	// 		delListHead = delListHead->rg_next;
	// 	}
	// 	delListHead->rg_next = removeItem->rg_start;
	// 	delListHead->rg_end += removeItem->rg_end;
	// }

	/*enlist the obsoleted memory region */
	enlist_vm_freerg_list(caller->mm, removedItem);

	return 0;
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
	return __free(proc, 0, reg_index);
}

// TODO Quang
/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
	uint32_t pte = mm->pgd[pgn];

	if (pte == 0)
	{
		return ERR_INVALID_ACCESS;
	}

	if (!PAGING_PAGE_PRESENT(pte))
	{ /* Page is not online, make it actively living */
		int vicpgn, swpfpn;
		// int vicfpn;
		// uint32_t vicpte;

		int tgtfpn = PAGING_SWAPFPN(pte); // the target frame storing our variable

		/* TODO: Play with your paging theory here */
		/* Find victim page */
		if (find_victim_page(caller->mm, &vicpgn, -1) != 0)
		{
			return ERR_SWAPPING;
		}
		uint32_t victim_pte = caller->mm->pgd[vicpgn];
		int victim_fpn = PAGING_FPN(victim_pte);

		/* Get free frame in MEMSWP */
		if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) != 0)
		{
			return ERR_SWAPPING;
		}

		/* Do swap frame from MEMRAM to MEMSWP and vice versa*/
		/* Copy victim frame to swap */
		__swap_cp_page(caller->mram, victim_fpn, caller->active_mswp, swpfpn); // potential param type mismatch
		/* Copy target frame from swap to mem */
		__swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, victim_fpn); // potential param type mismatch

#if IODUMP
		printf("Swapped frame RAM %d with frame SWAP %d while getpage\n", victim_fpn, tgtfpn);
#endif

		/* Target frame in SWAP is now free, add that to free frame list of SWAP */
		MEMPHY_put_freefp(caller->active_mswp, tgtfpn);

		/* Update page table */
		/* Update the victim page entry to SWAPPED */
		pte_set_swap(&mm->pgd[vicpgn], SWPTYP, swpfpn);

		/* Update its online status of the target page */
		/* Update the accessed page entry to PRESENT*/
		// pte_set_fpn() & mm->pgd[pgn];
		pte_set_fpn(&mm->pgd[pgn], victim_fpn);
		pte = mm->pgd[pgn];

		// enlist_pgn_node(&caller->mm->fifo_using_pgn, pgn);
		enlist_tail_pgn_node(&caller->mm->fifo_using_pgn, pgn);
	}

	*fpn = PAGING_FPN(pte);

	return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
	int pgn = PAGING_PGN(addr);
	int off = PAGING_OFFST(addr);
	int fpn;

	/* Get the page to MEMRAM, swap from MEMSWAP if needed */
	int getpage_status = pg_getpage(mm, pgn, &fpn, caller);
	if (getpage_status != 0)
		return getpage_status; /* invalid page access */

	int phyaddr = (fpn * PAGING_PAGESZ) + off;

	MEMPHY_read(caller->mram, phyaddr, data);

#if PAGING_DBG
	printf("Read from RAM successfully\n");
#endif // IODUMP

	return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
	int pgn = PAGING_PGN(addr);
	int off = PAGING_OFFST(addr);
	int fpn;

	/* Get the page to MEMRAM, swap from MEMSWAP if needed */
	int getpage_status = pg_getpage(mm, pgn, &fpn, caller);
	if (getpage_status != 0)
		return getpage_status; /* invalid page access */

	int phyaddr = (fpn * PAGING_PAGESZ) + off;

	MEMPHY_write(caller->mram, phyaddr, value);

#if PAGING_DBG
	printf("Write to RAM successfully\n");
#endif // IODUMP

	return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
	struct vm_rg_struct *currg = get_symbol_region_by_id(caller->mm, rgid);

	struct vm_area_struct *cur_vma = get_vma_by_index(caller->mm, vmaid);

	if (currg == NULL || cur_vma == NULL || currg->valid == 0) /* Invalid memory identify */
		return ERR_RW_UNDEFINED;

	if (currg->rg_start + offset < cur_vma->vm_start || currg->rg_start + offset >= cur_vma->sbrk)
		return ERR_INVALID_ACCESS;

	return pg_getval(caller->mm, currg->rg_start + offset, data, caller);
	;
}

/*pgwrite - PAGING-based read a region memory */
int pgread(
	struct pcb_t *proc, // Process executing the instruction
	uint32_t source,	// Index of source register
	uint32_t offset,	// Source address = [source] + [offset]
	uint32_t destination)
{
	BYTE data;
	int read_status = __read(proc, 0, source, offset, &data);

	destination = (uint32_t)data;
#if IODUMP
	if (read_status == 0)
		printf("read region=%d offset=%d value=%d\n", source, offset, data);
	else
		printf("read region=%d offset=%d value=null\n", source, offset);
#if PAGETBL_DUMP
	print_pgtbl(proc, 0, -1); // print max TBL
#endif						  // PAGETBL_DUMP
	MEMPHY_dump(proc->mram);
#endif // IODUMP

#if PAGING_ERR_DUMP
	int pid = proc->pid;
	int line = proc->pc + 1;
	if (read_status == ERR_INVALID_ACCESS)
	{
		printf("** ShopeeOS ERROR ** in PID: %d, at line: %d:\n ===> what: accessed invalid memory region, cannot read!\n", pid, line);
	}
	else if (read_status == ERR_SWAPPING)
	{
		printf("** ShopeeOS ERROR ** in PID: %d, at line: %d:\n ===> what: cannot bring required memory region from SWAP to RAM, cannot read!\n", pid, line);
	}
	else if (read_status != 0)
	{
		printf("** ShopeeOS ERROR ** in PID: %d, at line: %d:\n ===> what: undefined error, cannot read!\n", pid, line);
	}
#endif

	return read_status;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
	struct vm_rg_struct *currg = get_symbol_region_by_id(caller->mm, rgid);

	struct vm_area_struct *cur_vma = get_vma_by_index(caller->mm, vmaid);

	if (currg == NULL || cur_vma == NULL || currg->valid == 0) /* Invalid memory identify */
		return ERR_RW_UNDEFINED;

	if (currg->rg_start + offset < cur_vma->vm_start || currg->rg_start + offset >= cur_vma->sbrk)
		return ERR_INVALID_ACCESS;

	return pg_setval(caller->mm, currg->rg_start + offset, value, caller);
	;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
	struct pcb_t *proc,	  // Process executing the instruction
	BYTE data,			  // Data to be wrttien into memory
	uint32_t destination, // Index of destination register
	uint32_t offset)
{
#if IODUMP
	printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#if PAGETBL_DUMP
	print_pgtbl(proc, 0, -1); // print max TBL
#endif
	MEMPHY_dump(proc->mram);
#endif
	int write_status = __write(proc, 0, destination, offset, data);

#if PAGING_ERR_DUMP
	int pid = proc->pid;
	int line = proc->pc + 1;
	if (write_status == ERR_INVALID_ACCESS)
	{
		printf("** ShopeeOS ERROR ** in PID: %d, at line: %d:\n ===> what: accessed invalid memory region, cannot write!\n", pid, line);
	}
	else if (write_status == ERR_SWAPPING)
	{
		printf("** ShopeeOS ERROR ** in PID: %d, at line: %d:\n ===> what: cannot bring required memory region from SWAP to RAM, cannot write!\n", pid, line);
	}
	else if (write_status != 0)
	{
		printf("** ShopeeOS ERROR ** in PID: %d, at line: %d:\n ===> what: undefined error, cannot write!\n", pid, line);
	}
#endif

	return write_status;
}

// Thuan: TODO : Implement this somewhere
/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
	int pagenum, fpn;
	uint32_t pte;

	for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
	{
		pte = caller->mm->pgd[pagenum];

		if (!PAGING_PAGE_PRESENT(pte))
		{
			fpn = PAGING_FPN(pte);
			MEMPHY_put_freefp(caller->mram, fpn);
		}
		else
		{
			fpn = PAGING_SWAPFPN(pte);
			MEMPHY_put_freefp(caller->active_mswp, fpn);
		}
	}

	return 0;
}

// Quang
/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn, int exception_page)
{
	struct pgn_t *pgn_node = mm->fifo_using_pgn;
	if (!pgn_node)
	{
		return -1;
	}

	/* TODO: Implement the theorical mechanism to find the victim page */
	/* FIFO victim*/
	*retpgn = pgn_node->pgn;
	if (*retpgn != exception_page) // current return page number doesn't matches the exception, so don't care
	{
		mm->fifo_using_pgn = mm->fifo_using_pgn->pg_next;
	}
	else
	{ // current return page number matches the exception
#if PAGING_DBG
		printf("Matched exception page\n");
#endif
		if (pgn_node->pg_next == NULL) // cannot find another page
		{
			return -1;
		}
		else // take the next page number
		{
			pgn_node = pgn_node->pg_next;
			*retpgn = pgn_node->pgn;
			mm->fifo_using_pgn->pg_next = mm->fifo_using_pgn->pg_next->pg_next;
		}
	}

#if PAGING_DBG
	printf("Found victim page\n");
#endif
	free(pgn_node);

	return 0;
}

int count_available_victim(struct mm_struct *mm, int exception_page)
{
	struct pgn_t *pgn_node = mm->fifo_using_pgn;
	int count = 0;
	while (pgn_node)
	{
		if (pgn_node->pgn != exception_page)
		{
			count++;
		}
		pgn_node = pgn_node->pg_next;
	}

	return count;
}

// #endif
