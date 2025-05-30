// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, int vicfpn , int swpfpn)
{
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
    return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
/**
 * NOTE: cleaned
 * @note: deleted cur_vma->sbrk += alignedsz;
 * deleted newrg->rg_end = newrg->rg_start + alignedsz;
 * The newrg's end is not aligned to the page size
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if (!cur_vma) return NULL;
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  if (!newrg) return NULL;

  // Set the start and end of the new region
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;
  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
/**
 * NOTE: cleaned
 * @note: use loop to check vmaid with others
 * use OVERLAP()
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  if (vmastart >= vmaend) return -1;
  struct vm_area_struct *vma = caller->mm->mmap;
  if (!vma) return -1;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if (!cur_vma) return -1;

  // Check current VMA with every others  
  while (vma) {
    if (vma != cur_vma && OVERLAP(vmastart, vmaend, vma->vm_start, vma->vm_end))
      return -1;
    vma = vma->vm_next;
  }
  return 0; // No overlap
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
/**
 * NOTE: cleaned
 * @note: extended 
 * cur_vma->vm_end = old_end + inc_amt;
   cur_vma->sbrk += inc_amt;
   Delete free(newrg);
*/
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  if (!newrg) return -1;
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  int old_end = cur_vma->vm_end;

  // Validate overlap of the obtained region
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0) 
    return -1;
  // Extend the VMA
  cur_vma->vm_end = old_end + inc_amt;
  cur_vma->sbrk += inc_amt;

  // Map the memory to MEMRAM
  if (vm_map_ram(caller, area->rg_start, area->rg_end, old_end, incnumpage, newrg) < 0) {
    free(newrg);
    return -1;
  }
  return 0; // Success
}

// #endif
