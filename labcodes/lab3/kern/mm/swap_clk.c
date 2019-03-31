#include <defs.h>
#include <x86.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_clk.h>
#include <list.h>

/* [wikipedia]The simplest Page Replacement Algorithm(PRA) is a FIFO algorithm. The first-in, first-out
 * page replacement algorithm is a low-overhead algorithm that requires little book-keeping on
 * the part of the operating system. The idea is obvious from the name - the operating system
 * keeps track of all the pages in memory in a queue, with the most recent arrival at the back,
 * and the earliest arrival in front. When a page needs to be replaced, the page at the front
 * of the queue (the oldest page) is selected. While FIFO is cheap and intuitive, it performs
 * poorly in practical application. Thus, it is rarely used in its unmodified form. This
 * algorithm experiences Belady's anomaly.
 *
 * Details of FIFO PRA
 * (1) Prepare: In order to implement FIFO PRA, we should manage all swappable pages, so we can
 *              link these pages into pra_list_head according the time order. At first you should
 *              be familiar to the struct list in list.h. struct list is a simple doubly linked list
 *              implementation. You should know howto USE: list_init, list_add(list_add_after),
 *              list_add_before, list_del, list_next, list_prev. Another tricky method is to transform
 *              a general list struct to a special struct (such as struct page). You can find some MACRO:
 *              le2page (in memlayout.h), (in future labs: le2vma (in vmm.h), le2proc (in proc.h),etc.
 */

list_entry_t pra_list_head;
/*
 * (2) _clk_init_mm: init pra_list_head and let  mm->sm_priv point to the addr of pra_list_head.
 *              Now, From the memory control struct mm_struct, we can access FIFO PRA
 */
static int
_clk_init_mm(struct mm_struct *mm)
{     
     list_init(&pra_list_head);
     mm->sm_priv = &pra_list_head;
     //cprintf(" mm->sm_priv %x in clk_init_mm\n",mm->sm_priv);
     return 0;
}
/*
 * (3)_clk_map_swappable: According FIFO PRA, we should link the most recent arrival page at the back of pra_list_head qeueue
 */
static int
_clk_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    list_entry_t *entry=&(page->pra_page_link);
 
    assert(entry != NULL && head != NULL);
    //record the page access situlation
    /*LAB3 EXERCISE 2: 2016011395*/ 
    //(1)link the most recent arrival page at the back of the pra_list_head qeueue.
	if(clk_ptr == NULL){
		list_add_before(head,entry);
		clk_ptr = head;
	}else{
		list_add_before(clk_ptr,entry);
	}
	return 0;
}
/*
 *  (4)_clk_swap_out_victim: According FIFO PRA, we should unlink the  earliest arrival page in front of pra_list_head qeueue,
 *                            then assign the value of *ptr_page to the addr of this page.
 */
static int
_clk_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick)
{
     list_entry_t *head=(list_entry_t*) mm->sm_priv;
         assert(head != NULL);
     assert(in_tick==0);
     /* Select the victim */
     /*LAB3 EXERCISE 2: 2016011395*/ 
	 if(clk_ptr == NULL){
		clk_ptr = head->next;
	 }
	 if(clk_ptr == head){
		clk_ptr = clk_ptr->next;
	 }
	 struct Page* p_clk = le2page(clk_ptr,pra_page_link);
	 pte_t* pte_clk = get_pte(mm->pgdir,p_clk->pra_vaddr,0);
	 while(1){
		if(*pte_clk&PTE_A){
			*pte_clk = *pte_clk&(~PTE_A);
			tlb_invalidate(mm->pgdir,p_clk->pra_vaddr);
		}else{
			if(*pte_clk&PTE_D){
				swapfs_write((p_clk->pra_vaddr/PGSIZE+1)<<8,p_clk);
				*pte_clk &= ~PTE_D;
			}else{
				*ptr_page = p_clk;
				clk_ptr = clk_ptr->next;
				break;
			}
		}
		clk_ptr = clk_ptr->next;
		if(clk_ptr == head){
			clk_ptr = clk_ptr->next;
		}
		p_clk = le2page(clk_ptr,pra_page_link);
		pte_clk = get_pte(mm->pgdir,p_clk->pra_vaddr,0);
	 }

	 
	 return 0;
}

static int
_clk_check_swap(void) {
	//copied from twd2
	pde_t *pgdir = KADDR((pde_t *)rcr3());
    int i;
	for (i = 0; i < 4; ++i) {
        pte_t *ptep = get_pte(pgdir, (i + 1) * 0x1000, 0);
        assert(*ptep & PTE_P);
        assert(swapfs_write(((i + 1) * 0x1000 / PGSIZE + 1) << 8, pte2page(*ptep)) == 0);
        *ptep &= ~(PTE_A | PTE_D);
        tlb_invalidate(pgdir, (i + 1) * 0x1000);
    }
    assert(pgfault_num == 4);
    cprintf("read Virt Page c in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x3000 == 0x0c);
    assert(pgfault_num == 4);
    cprintf("write Virt Page a in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num == 4);
    cprintf("read Virt Page d in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x4000 == 0x0d);
    assert(pgfault_num == 4);
    cprintf("write Virt Page b in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x2000 == 0x0b);
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num == 4);
    cprintf("read Virt Page e in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x5000 == 0x00);
    assert(pgfault_num == 5);
    cprintf("read Virt Page b in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x2000 == 0x0b);
    assert(pgfault_num == 5);
    cprintf("write Virt Page a in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num == 5);
    cprintf("read Virt Page b in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x2000 == 0x0b);
    assert(pgfault_num == 5);
    cprintf("read Virt Page c in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x3000 == 0x0c);
    assert(pgfault_num == 6);
    cprintf("read Virt Page d in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x4000 == 0x0d);
    assert(pgfault_num == 7);
    cprintf("write Virt Page e in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x5000 == 0x00);
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num == 7);
    cprintf("write Virt Page a in extclk_dirty_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num == 7);
    return 0; 
}


static int
_clk_init(void)
{
    return 0;
}

static int
_clk_set_unswappable(struct mm_struct *mm, uintptr_t addr)
{
    return 0;
}

static int
_clk_tick_event(struct mm_struct *mm)
{ return 0; }


struct swap_manager swap_manager_clk =
{
     .name            = "clk swap manager",
     .init            = &_clk_init,
     .init_mm         = &_clk_init_mm,
     .tick_event      = &_clk_tick_event,
     .map_swappable   = &_clk_map_swappable,
     .set_unswappable = &_clk_set_unswappable,
     .swap_out_victim = &_clk_swap_out_victim,
     .check_swap      = &_clk_check_swap,
};
