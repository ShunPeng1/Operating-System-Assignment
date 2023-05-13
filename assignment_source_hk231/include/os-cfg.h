#ifndef OSCFG_H
#define OSCFG_H

#define MLQ_SCHED 1
#define MAX_PRIO 140

#define MM_PAGING

/*SYNCHRONIZATION*/
#define SYNC_SCHED 1 // For scheduler
#define SYNC_MM 1 // For Memphy

// #define MM_FIXED_MEMSZ
#define VMDBG 0
#define MMDBG 0

/*SCHEDULER*/
#define SCHED_DUMP 0

/*PAGING*/
#define IODUMP 1
#define PAGETBL_DUMP 1

#define PAGING_ERR_DUMP 1

/*DEBUG SECTION*/
#define SCHED_DBG 0
#define PAGING_DBG 0


#endif
