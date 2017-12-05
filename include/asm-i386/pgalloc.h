#ifndef _I386_PGALLOC_H
#define _I386_PGALLOC_H

#include <linux/config.h>
#include <asm/processor.h>
#include <asm/fixmap.h>
#include <linux/threads.h>

#define pgd_quicklist (current_cpu_data.pgd_quick)
#define pmd_quicklist (current_cpu_data.pmd_quick)
#define pte_quicklist (current_cpu_data.pte_quick)
#define pgtable_cache_size (current_cpu_data.pgtable_cache_sz)

#if CONFIG_X86_PAE
# include <asm/pgalloc-3level.h>
#else
# include <asm/pgalloc-2level.h>
#endif

/*
 * Allocate and free page tables. The xxx_kernel() versions are
 * used to allocate a kernel page table - this turns on ASN bits
 * if any.
 */

extern __inline__ pgd_t *get_pgd_slow(void)
{
	pgd_t *ret = (pgd_t *)__get_free_page(GFP_KERNEL);

	if (ret) {
#if CONFIG_X86_PAE
		int i;
		for (i = 0; i < USER_PTRS_PER_PGD; i++)
			__pgd_clear(ret + i);
#else
		memset(ret, 0, USER_PTRS_PER_PGD * sizeof(pgd_t));
#endif
		memcpy(ret + USER_PTRS_PER_PGD, swapper_pg_dir + USER_PTRS_PER_PGD, (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
	}
	return ret;
}

extern __inline__ pgd_t *get_pgd_fast(void)
{
	unsigned long *ret;

	if ((ret = pgd_quicklist) != NULL) {
		pgd_quicklist = (unsigned long *)(*ret);
		ret[0] = 0;
		pgtable_cache_size--;
	} else
		ret = (unsigned long *)get_pgd_slow();
	return (pgd_t *)ret;
}

extern __inline__ void free_pgd_fast(pgd_t *pgd)
{
	*(unsigned long *)pgd = (unsigned long) pgd_quicklist;
	pgd_quicklist = (unsigned long *) pgd;
	pgtable_cache_size++;
}

extern __inline__ void free_pgd_slow(pgd_t *pgd)
{
	free_page((unsigned long)pgd);
}

extern pte_t *get_pte_slow(pmd_t *pmd, unsigned long address_preadjusted);
extern pte_t *get_pte_kernel_slow(pmd_t *pmd, unsigned long address_preadjusted);

extern __inline__ pte_t *get_pte_fast(void)
{
	unsigned long *ret;

	if((ret = (unsigned long *)pte_quicklist) != NULL) {
		pte_quicklist = (unsigned long *)(*ret);
		ret[0] = ret[1];
		pgtable_cache_size--;
	}
	return (pte_t *)ret;
}

extern __inline__ void free_pte_fast(pte_t *pte)
{
	*(unsigned long *)pte = (unsigned long) pte_quicklist;
	pte_quicklist = (unsigned long *) pte;
	pgtable_cache_size++;
}

extern __inline__ void free_pte_slow(pte_t *pte)
{
	free_page((unsigned long)pte);
}

#define pte_free_kernel(pte)    free_pte_slow(pte)
#define pte_free(pte)	   free_pte_slow(pte)
#define pgd_free(pgd)	   free_pgd_slow(pgd)
#define pgd_alloc()	     get_pgd_fast()

extern inline pte_t * pte_alloc_kernel(pmd_t * pmd, unsigned long address)
{
	if (!pmd)
		BUG();
	address = (address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1);
	if (pmd_none(*pmd)) {
		pte_t * page = (pte_t *) get_pte_fast();

		if (!page)
			return get_pte_kernel_slow(pmd, address);
		set_pmd(pmd, __pmd(_KERNPG_TABLE + __pa(page)));
		return page + address;
	}
	if (pmd_bad(*pmd)) {
		__handle_bad_pmd_kernel(pmd);
		return NULL;
	}
	return (pte_t *) pmd_page(*pmd) + address;
}

extern inline pte_t * pte_alloc(pmd_t * pmd, unsigned long address)
{
	address = (address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1); /* 先将给定的地址转换成其所属页面的下标 */

	if (pmd_none(*pmd)) /* 如果指针pmd所指向的目录项为空，转到getnew处封呢排额一个页面表 */
		goto getnew;
	if (pmd_bad(*pmd))
		goto fix;
	return (pte_t *)pmd_page(*pmd) + address;
getnew:
{
    /*
     * 这里调用get_pte_fast()来分配一个页面。内核对页面的分配做了一些优化。当释放一个页面表时，内核将释放的页面表先保存在一个缓冲池里，
     * 而先不将其物理页释放。只有在缓冲池已满的情况下才真的将物理内存释放。这样，在要分配一个页面表时，就可以先看以下get_pte_fast().要是缓冲池已经空了，
     * 那就只好通过get_pte_kernel_slow()来进行分配了，这种分配方法有时候可能会比较慢，因为：分配一个物理内存页面用作页面时，可能物理内存已经用完了，
     * 需要把内存中已经占用的页面交换到磁盘中去
     */
	unsigned long page = (unsigned long) get_pte_fast();
	if (!page)
		return get_pte_slow(pmd, address);

    /* 分配到一个页面表以后，就通过set_pmd()中将其起始地址连同一些属性标志位一起写入中间目录项，对于i386实际上写入到了pgd中 */
	set_pmd(pmd, __pmd(_PAGE_TABLE + __pa(page)));

	return (pte_t *)page + address;
}
fix:
	__handle_bad_pmd(pmd);
	return NULL;
}

/*
 * allocating and freeing a pmd is trivial: the 1-entry pmd is
 * inside the pgd, so has no extra memory associated with it.
 * (In the PAE case we free the page.)
 */
#define pmd_free(pmd)	   free_pmd_slow(pmd)

#define pmd_free_kernel		pmd_free
#define pmd_alloc_kernel	pmd_alloc

extern int do_check_pgt_cache(int, int);

/*
 * TLB flushing:
 *
 *  - flush_tlb() flushes the current mm struct TLBs
 *  - flush_tlb_all() flushes all processes TLBs
 *  - flush_tlb_mm(mm) flushes the specified mm context TLB's
 *  - flush_tlb_page(vma, vmaddr) flushes one page
 *  - flush_tlb_range(mm, start, end) flushes a range of pages
 *  - flush_tlb_pgtables(mm, start, end) flushes a range of page tables
 *
 * ..but the i386 has somewhat limited tlb flushing capabilities,
 * and page-granular flushes are available only on i486 and up.
 */

#ifndef CONFIG_SMP

#define flush_tlb() __flush_tlb()
#define flush_tlb_all() __flush_tlb_all()
#define local_flush_tlb() __flush_tlb()

static inline void flush_tlb_mm(struct mm_struct *mm)
{
	if (mm == current->active_mm)
		__flush_tlb();
}

static inline void flush_tlb_page(struct vm_area_struct *vma,
	unsigned long addr)
{
	if (vma->vm_mm == current->active_mm)
		__flush_tlb_one(addr);
}

static inline void flush_tlb_range(struct mm_struct *mm,
	unsigned long start, unsigned long end)
{
	if (mm == current->active_mm)
		__flush_tlb();
}

#else

#include <asm/smp.h>

#define local_flush_tlb() \
	__flush_tlb()

extern void flush_tlb_all(void);
extern void flush_tlb_current_task(void);
extern void flush_tlb_mm(struct mm_struct *);
extern void flush_tlb_page(struct vm_area_struct *, unsigned long);

#define flush_tlb()	flush_tlb_current_task()

static inline void flush_tlb_range(struct mm_struct * mm, unsigned long start, unsigned long end)
{
	flush_tlb_mm(mm);
}

#define TLBSTATE_OK	1
#define TLBSTATE_LAZY	2

struct tlb_state
{
	struct mm_struct *active_mm;
	int state;
};
extern struct tlb_state cpu_tlbstate[NR_CPUS];


#endif

extern inline void flush_tlb_pgtables(struct mm_struct *mm,
				      unsigned long start, unsigned long end)
{
	/* i386 does not keep any page table caches in TLB */
}

#endif /* _I386_PGALLOC_H */
