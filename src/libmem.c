/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c 
 */

#include "string.h"
#include "mm.h"
#include "syscall.h"
#include "libmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/**
 * @NOTE: helper function to print the page table
 */
void helper(struct pcb_t *proc) {
  int pgit = 0;
  int pgn = PAGING_PGN(0);
  int pgnum = PAGING_PAGE_ALIGNSZ(proc->mm->mmap->sbrk) / PAGING_PAGESZ; // only 1 vma
  for (pgit = 0; pgit < pgnum; pgit++)
  {
    uint32_t pte = proc->mm->pgd[pgn + pgit];
    printf("Page Number: %d -> Frame Number: %d\n", pgn + pgit, PAGING_PTE_FPN(pte));
  }
}

int MEMPHY_dump1(struct memphy_struct *mp)
{
  /*TODO dump memphy contnt mp->storage
   *     for tracing the memory content
   */
   if (mp == NULL || mp->storage == NULL) {
      printf("MEMPHY_dump: Invalid memory structure.\n");
      return -1;
  }

  printf("================================================================\n");
  printf("===== PHYSICAL MEMORY DUMP =====\n");

  for (int i = 0; i < mp->maxsz; i++) {
      if (mp->storage[i] != 0) { // Only print non-zero bytes
          printf("BYTE %08x: %d\n", i, mp->storage[i]);
      }
  }

  printf("===== PHYSICAL MEMORY END-DUMP =====\n");
  printf("================================================================\n");

  return 0;
}

void alloc_dump(struct pcb_t *caller, int size, int alloc_addr) {
  // if (caller->pid != 3) return;
  pthread_mutex_lock(&mmvm_lock);
  printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
  int start_page = PAGING_PGN(get_vma_by_num(caller->mm, 0)->vm_start);
  int end_page = PAGING_PGN(get_vma_by_num(caller->mm, 0)->vm_end);
  int num_regions = end_page - start_page;
  printf("PID=%d - Region=%d - Address=%08x - Size=%d byte\n",
         caller->pid, num_regions, alloc_addr,
         size);
  print_pgtbl(caller, 0, -1);

  // Iteratively print all mapped pages
  // for (int i = start_page; i < end_page; i++) {
  //   printf("Page Number: %d -> Frame Number: %d\n",
  //          i, PAGING_PTE_FPN(caller->mm->pgd[i]));
  // }
  helper(caller);
  printf("================================================================\n");
  pthread_mutex_unlock(&mmvm_lock);
}

void dealloc_dump(struct pcb_t *caller) {
  // if (caller->pid != 3) return;
  pthread_mutex_lock(&mmvm_lock);
  int start_page = PAGING_PGN(get_vma_by_num(caller->mm, 0)->vm_start);
  int end_page = PAGING_PGN(get_vma_by_num(caller->mm, 0)->vm_end);
  int num_regions = end_page - start_page;
  printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n");
  printf("PID=%d - Region=%d\n", caller->pid, num_regions);
  print_pgtbl(caller, 0, -1);
  // for (int i = start_page; i < end_page; i++) {
  //   printf("Page Number: %d -> Frame Number: %d\n",
  //          i, PAGING_PTE_FPN(caller->mm->pgd[i]));
  // }
  helper(caller);
  printf("================================================================\n");
  pthread_mutex_unlock(&mmvm_lock);
}

void write_dump(struct pcb_t *caller, int offset, BYTE value, int rgid) {
  pthread_mutex_lock(&mmvm_lock);
  printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
  printf("write region=%d offset=%d value=%d\n",
         rgid, offset, value);
  print_pgtbl(caller, 0, -1);       
  helper(caller);
  MEMPHY_dump1(caller->mram);
  printf("================================================================\n");
  pthread_mutex_unlock(&mmvm_lock);
}

void read_dump(struct pcb_t *caller, int offset, BYTE value, int rgid) {
  pthread_mutex_lock(&mmvm_lock);
  printf("===== PHYSICAL MEMORY AFTER READING =====\n");
  printf("read region=%d offset=%d value=%d\n",
         rgid, offset, value);
  print_pgtbl(caller, 0, -1);       
  helper(caller);
  MEMPHY_dump1(caller->mram);
  printf("================================================================\n");
  pthread_mutex_unlock(&mmvm_lock);
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
/**
 * NOTE: cleaned
 * @note: !suitable space in free region list, increase the VMA limit
 * deleted: 
 *  cur_vma->sbrk += inc_sz;
    cur_vma->vm_end = cur_vma->sbrk;
    already done by inc_vma_limit
 * deleted:
    int inc_sz = PAGING_PAGE_ALIGNSZ(size);
    The unaligned size is passed into inc_vma_limit and aligned later
 * modiified freerg logic of updating start and end
 * add: add newly created spare region to free region list
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  struct vm_rg_struct rgnode;
  // Attempt to find a free region in the virtual memory area
  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    pthread_mutex_lock(&mmvm_lock);
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
    *alloc_addr = rgnode.rg_start;
    pthread_mutex_unlock(&mmvm_lock);
    alloc_dump(caller, size, *alloc_addr);
    return 0;
  }

  // If no free region is found, attempt to increase the VMA limit
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if (!cur_vma) return -1;

  int inc_sz = size;
  struct sc_regs regs;
  regs.a1 = SYSMEM_INC_OP;
  regs.a2 = vmaid;
  regs.a3 = inc_sz;

  if (syscall(caller, 17, &regs) < 0) {
    printf("Failed to increase VMA limit\n");
    return -1;
  }

  pthread_mutex_lock(&mmvm_lock);
  caller->mm->symrgtbl[rgid].rg_start = cur_vma->sbrk - PAGING_PAGE_ALIGNSZ(inc_sz);
  caller->mm->symrgtbl[rgid].rg_end = caller->mm->symrgtbl[rgid].rg_start + size;
  *alloc_addr = caller->mm->symrgtbl[rgid].rg_start;

  if (cur_vma->sbrk - caller->mm->symrgtbl[rgid].rg_end > 0) {
    struct vm_rg_struct *new_rg = malloc(sizeof(struct vm_rg_struct));
    new_rg->rg_start = caller->mm->symrgtbl[rgid].rg_end;
    new_rg->rg_end = cur_vma->sbrk;
    enlist_vm_freerg_list(caller->mm, new_rg);
  }
  
  pthread_mutex_unlock(&mmvm_lock);
#ifdef IODUMP
  alloc_dump(caller, size, *alloc_addr);
#endif
  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
/**
 * NOTE: cleaned
 * @note: deleted resetting rg_start and rg_end
 * rgnode->rg_start = -1;
 * rgnode->rg_end = -1;
 * This causes new process to allocate new region instead of using the freed one
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;

  // Retrieve the memory region by ID
  struct vm_rg_struct *rgnode = get_symrg_byid(caller->mm, rgid);
  if (!rgnode || (rgnode->rg_start == -1 && rgnode->rg_end == -1))
    return -1;

  // Add the region back to the free region list
  if (enlist_vm_freerg_list(caller->mm, rgnode) < 0)
    return -1;
#ifdef IODUMP
  dealloc_dump(caller);
#endif
  return 0;
}

/*liballoc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*libfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int libfree(struct pcb_t *proc, uint32_t reg_index)
{
  /* TODO Implement free region */

  /* By default using vmaid = 0 */
  return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
/**
 * NOTE: cleaned
 * @note: directly call the swap functions instead of syscall
 * @note: modified swaptype from 1 to 0 to fix 00000000: c0000001
 *  pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = mm->pgd[pgn];

  if (!PAGING_PAGE_PRESENT(pte))
  {
    int vicpgn, swpfpn, vicfpn, tgtfpn;

    // Find a victim page (RAM -> SWAP)
    if (find_victim_page(mm, &vicpgn) < 0) return -1;
    // Get a free frame in MEMSWP
    if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) < 0) return -1;

    // Swap victim page to MEMSWP
    vicfpn = PAGING_FPN(mm->pgd[vicpgn]);
    // __mm_swap_page(caller, vicfpn, swpfpn);

    struct sc_regs regs;
    regs.a1 = SYSMEM_SWP_OP;
    regs.a2 = vicpgn;
    regs.a3 = swpfpn;
    if (syscall(caller, 17, &regs) < 0) return -1;

    pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);

    // Get a free frame in MEMRAM
    if (MEMPHY_get_freefp(caller->mram, &tgtfpn) < 0) return -1;

    // Swap target frame from MEMSWP to MEMRAM
    regs.a1 = SYSMEM_SWP_OP;
    regs.a2 = tgtfpn;
    regs.a3 = PAGING_PTE_SWP(pte);
    if (syscall(caller, 17, &regs) < 0) return -1;

    pte_set_fpn(&mm->pgd[pgn], tgtfpn);

    // Enlist the page in the FIFO list
    enlist_pgn_node(&mm->fifo_pgn, pgn);
  }

  *fpn = PAGING_FPN(mm->pgd[pgn]);
  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
/**
 * NOTE: cleaned
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  // Get the page into MEMRAM, swapping from MEMSWAP if needed
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; // Invalid page access

  // Calculate the physical address
  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  // Perform a syscall to read the value from the physical address
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;
  regs.a2 = phyaddr;
  regs.a3 = 0;

  if (syscall(caller, 17, &regs) < 0)
    return -1; // Syscall failed

  // Update the data with the value read
  *data = (BYTE)regs.a3;

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
/**
 * NOTE: cleaned
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  // Get the page into MEMRAM, swapping from MEMSWAP if needed
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; // Invalid page access

  // Calculate the physical address
  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  // Perform a syscall to write the value to the physical address
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE;
  regs.a2 = phyaddr;
  regs.a3 = value;

  if (syscall(caller, 17, &regs) < 0)
    return -1; // Syscall failed

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
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
    return -1;

  pg_getval(caller->mm, currg->rg_start + offset, data, caller);

#ifdef IODUMP
  read_dump(caller, offset, *data, rgid);
#endif  
  return 0;
}

/*libread - PAGING-based read a region memory */
int libread(
    struct pcb_t *proc, // Process executing the instruction
    uint32_t source,    // Index of source register
    uint32_t offset,    // Source address = [source] + [offset]
    uint32_t* destination)
{
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);

  /* TODO update result of reading action*/
  if (val == 0) {
    *destination = (uint32_t)data;
  }
  //destination 
#ifdef IODUMP
  // printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return val;
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
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
    return -1;

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);
#ifdef IODUMP
  write_dump(caller, offset, value, rgid);
#endif
  return 0;
}

/*libwrite - PAGING-based write a region memory */
int libwrite(
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    uint32_t offset)
{
#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  // MEMPHY_dump(proc->mram);
#endif

  return __write(proc, 0, destination, offset, data);
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_PTE_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);    
    }
  }

  return 0;
}


/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
/**
 * @NOTE: modifed to FIFO
 */
int find_victim_page(struct mm_struct *mm, int *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn;
  struct pgn_t *prev = NULL;
  if(pg == NULL) return -1;

  /* TODO: Implement the theorical mechanism to find the victim page */
  while (pg->pg_next != NULL) {
    prev = pg;
    pg = pg->pg_next;
  }
  *retpgn = pg->pgn;
  prev->pg_next = NULL;

  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
/**
 * NOTE: cleaned
 * @note: add return when failed allocation
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if (!cur_vma) return -1;
  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
  if (!rgit) return -1;

  while (rgit != NULL) {
    if (rgit->rg_end - rgit->rg_start >= size) {
      // Found a suitable region
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      // Update the free region to reflect the allocated space
      rgit->rg_start += size;

      // If the free region is fully used, remove it from the list
      if (rgit->rg_start >= rgit->rg_end) {
        cur_vma->vm_freerg_list = rgit->rg_next;
        free(rgit);
      }
      return 0; // Success
    }
    rgit = rgit->rg_next;
  } 

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;
  return -1;
}

//#endif
