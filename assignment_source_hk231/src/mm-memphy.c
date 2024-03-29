// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include "os-mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/*
 *  MEMPHY_move_cursor - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_move_cursor(struct memphy_struct *mp, int offset)
{
	int numstep = 0;

	mp->cursor = 0;
	while (numstep < offset && numstep < mp->maxsz)
	{
		/* Traverse sequentially */
		mp->cursor = (mp->cursor + 1) % mp->maxsz;
		numstep++;
	}

	return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, int addr, BYTE *value)
{
	if (mp == NULL)
		return -1;

	if (!mp->rdmflg)
		return -1; /* Not compatible mode for sequential read */
#if SYNC_MM
	pthread_mutex_lock(&mp->lock_storage);
#endif // SYNC_MM
	MEMPHY_move_cursor(mp, addr);
	*value = (BYTE)mp->storage[addr];
#if SYNC_MM
	pthread_mutex_unlock(&mp->lock_storage);
#endif // SYNC_MM
	return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct *mp, int addr, BYTE *value)
{
	if (mp == NULL)
		return -1;


	if (mp->rdmflg)
	{
#if SYNC_MM
		pthread_mutex_lock(&mp->lock_storage);
#endif // SYNC_MM
		*value = mp->storage[addr];
#if SYNC_MM
		pthread_mutex_unlock(&mp->lock_storage);
#endif // SYNC_MM
	}
	else /* Sequential access device */{
		return MEMPHY_seq_read(mp, addr, value);
	}
		

	return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct *mp, int addr, BYTE value)
{

	if (mp == NULL)
		return -1;

	if (!mp->rdmflg)
		return -1; /* Not compatible mode for sequential read */
#if SYNC_MM
	pthread_mutex_lock(&mp->lock_storage);
#endif // SYNC_MM
	MEMPHY_move_cursor(mp, addr);
	mp->storage[addr] = value;
#if SYNC_MM
	pthread_mutex_unlock(&mp->lock_storage);
#endif // SYNC_MM	
	return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct *mp, int addr, BYTE data)
{
	
	if (mp == NULL)
		return -1;
	
	if (mp->rdmflg)
	{
#if SYNC_MM
		pthread_mutex_lock(&mp->lock_storage);
#endif // SYNC_MM
		mp->storage[addr] = data;
#if SYNC_MM
		pthread_mutex_unlock(&mp->lock_storage);
#endif // SYNC_MM
	}
	else /* Sequential access device */
	{
		return MEMPHY_seq_write(mp, addr, data);
	}

	return 0;
}

/*
 *  MEMPHY_format- init format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
	/* This setting come with fixed constant PAGESZ */
	int numfp = mp->maxsz / pagesz;
	struct framephy_struct *newfst, *fst;
	int iter = 0;

	if (numfp <= 0)
		return -1;

	/* Init head of free framephy list */
	fst = malloc(sizeof(struct framephy_struct));
	fst->fpn = iter;
	mp->free_fp_list = fst;
	mp->used_fp_list = NULL;
	
	mp->num_of_free_frame = numfp;
	mp->num_of_used_frame = 0;

	/* We have free frame list with first element, fill in the rest num-1 element member*/
	for (iter = 1; iter < numfp; iter++)
	{
		newfst = malloc(sizeof(struct framephy_struct));
		newfst->fpn = iter;
		newfst->fp_next = NULL;
		fst->fp_next = newfst;
		fst = newfst;
	}

	return 0;
}

/*
 *  MEMPHY_deformat- delete format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_deformat(struct memphy_struct *mp)
{
    struct framephy_struct *fst = mp->free_fp_list;
    struct framephy_struct *temp;

    while (fst != NULL)
    {
        temp = fst;
        fst = fst->fp_next;
        free(temp);
    }

    mp->free_fp_list = NULL;
	return 0;
}

// Return the free frame number id in retfpn, from the head of free frame list and free it
int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn)
{
#if SYNC_MM
	pthread_mutex_lock(&mp->lock_free_fp);
#endif // SYNC_MM
	struct framephy_struct *fp = mp->free_fp_list;

	if (fp == NULL){
#if SYNC_MM
		pthread_mutex_unlock(&mp->lock_free_fp);
#endif // SYNC_MM
		return -1;
	}

	*retfpn = fp->fpn;
	mp->free_fp_list = fp->fp_next;
	mp->num_of_free_frame--;
	/* MEMPHY is iteratively used up until its exhausted
	 * No garbage collector acting then it not been released
	 */
#if SYNC_MM
	pthread_mutex_unlock(&mp->lock_free_fp);
#endif // SYNC_MM
	free(fp);
	return 0;
}

int MEMPHY_dump(struct memphy_struct *mp)
{
	/*TODO dump memphy contnt mp->storage
	 *     for tracing the memory content
	 */
#if SYNC_MM
	pthread_mutex_lock(&mp->lock_used_fp);
#endif // SYNC_MM

	struct framephy_struct* used_framed = mp->used_fp_list;
	while (used_framed)
	{
		printf("Used frame page number %d\n", used_framed->fpn);
		used_framed = used_framed->fp_next;
	}
#if SYNC_MM
	pthread_mutex_unlock(&mp->lock_used_fp);
	pthread_mutex_lock(&mp->lock_storage);
#endif // SYNC_MM
	for (int byte_idx = 0; byte_idx < mp->maxsz; byte_idx++)
	{
		if (mp->storage[byte_idx] != 0)
		{
			printf("Offset %d: %d\n", byte_idx, mp->storage[byte_idx]);
		}
	}
#if SYNC_MM
	pthread_mutex_unlock(&mp->lock_storage);
#endif // SYNC_MM	
	return 0;
}

// Create a framephy_struct to the head of free frame linked list, fpn is the frame number id
int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{
	struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));
#if SYNC_MM
	pthread_mutex_lock(&mp->lock_free_fp);
#endif // SYNC_MM
	struct framephy_struct *fp = mp->free_fp_list;

	/* Create new node with value fpn */
	newnode->fpn = fpn;
	newnode->fp_next = fp;
	mp->free_fp_list = newnode;

	mp->num_of_free_frame++;
#if SYNC_MM
	pthread_mutex_unlock(&mp->lock_free_fp);
#endif // SYNC_MM
	return 0;
}

int MEMPHY_clean_frame(struct memphy_struct *mp, int fpn)
{
#if SYNC_MM
	pthread_mutex_lock(&mp->lock_storage);
#endif // SYNC_MM	
	memset(mp->storage + fpn * PAGING_PAGESZ, 0, PAGING_PAGESZ);
#if SYNC_MM
	pthread_mutex_unlock(&mp->lock_storage);
#endif // SYNC_MM
	return 0;
}

// Concat a frame linked list to used frame list
int MEMPHY_concat_usedfp(struct memphy_struct *mp, struct framephy_struct *start_node)
{
#if SYNC_MM
	pthread_mutex_lock(&mp->lock_used_fp);
#endif // SYNC_MM
	if (mp == NULL || start_node == NULL)
	{
#if SYNC_MM
		pthread_mutex_unlock(&mp->lock_used_fp);
#endif // SYNC_MM
		return -1;
	}

	// Find the last node in the used_fp_list
	struct framephy_struct *last_node = mp->used_fp_list;
	while (last_node != NULL && last_node->fp_next != NULL)
	{
		last_node = last_node->fp_next;
	}

	// Concatenate the start_node to the end of the used_fp_list
	if (last_node == NULL)
	{
		mp->used_fp_list = start_node;
	}
	else
	{
		last_node->fp_next = start_node;
	}
#if SYNC_MM
	pthread_mutex_unlock(&mp->lock_used_fp);
#endif // SYNC_MM
	return 0;
}

int MEMPHY_count_free_frame(struct memphy_struct * mp)
{
	int count;
#if SYNC_MM
	pthread_mutex_lock(&mp->lock_free_fp);
#endif // SYNC_MM
	count = mp->num_of_free_frame;
#if SYNC_MM
	pthread_mutex_unlock(&mp->lock_free_fp);
#endif // SYNC_MM

	return count;
}

/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
	mp->storage = (BYTE *)calloc(max_size, sizeof(BYTE));
	mp->maxsz = max_size;

	MEMPHY_format(mp, PAGING_PAGESZ);
#if SYNC_MM
	pthread_mutex_init(&mp->lock_free_fp, NULL);
	pthread_mutex_init(&mp->lock_used_fp, NULL);
	pthread_mutex_init(&mp->lock_storage, NULL);
#endif // SYNC_MM
	mp->rdmflg = (randomflg != 0) ? 1 : 0;

	if (!mp->rdmflg) /* Not Ramdom acess device, then it serial device*/
		mp->cursor = 0;

	return 0;
}

int delete_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
	free(mp->storage);
	
	//MEMPHY_deformat(mp);
#if SYNC_MM
	pthread_mutex_destroy(&mp->lock_free_fp);
	pthread_mutex_destroy(&mp->lock_used_fp);
	pthread_mutex_destroy(&mp->lock_storage);
#endif // SYNC_MM

	return 0;
}

// #endif
