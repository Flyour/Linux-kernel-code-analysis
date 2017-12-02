#ifndef _I386_PGTABLE_2LEVEL_H
#define _I386_PGTABLE_2LEVEL_H

/*
 * traditional i386 two-level paging structure:
 */

#define PGDIR_SHIFT	22
#define PTRS_PER_PGD	1024
/*
 * PGDIR_SHIFT 表示线性地址中PGD下标位段的起始位置，即是bit22位(第23位)，由于PGD是线性地址中的最高位段，
 * 那么就可以的到PGD位段是从23位到32位 。 一共是10位.
 * PTRS_PER_PGD 即是pointers_per_pgd，代表每一个PGD表中指针的个数为1024.
 * 显然，1024与线性地址中PGD位段的长度(10位)是相符的。
 */


/*
 * the i386 is two-level, so we don't really have any
 * PMD directory physically.
 */
#define PMD_SHIFT	22
#define PTRS_PER_PMD	1
/*
 * PMD_SHIFT 定义为22,就意味着，与PGD的起始位置相同，就是说，线性地址中PMD的位段长度为0.
 * 而PTRS_PER_PMD为1,则表示每个PMD表中只有一个表项，2^0=1.
 */


#define PTRS_PER_PTE	1024

#define pte_ERROR(e) \
	printk("%s:%d: bad pte %08lx.\n", __FILE__, __LINE__, (e).pte_low)
#define pmd_ERROR(e) \
	printk("%s:%d: bad pmd %08lx.\n", __FILE__, __LINE__, pmd_val(e))
#define pgd_ERROR(e) \
	printk("%s:%d: bad pgd %08lx.\n", __FILE__, __LINE__, pgd_val(e))

/*
 * The "pgd_xxx()" functions here are trivial for a folded two-level
 * setup: the pgd is never bad, and a pmd always exists (as it's folded
 * into the pgd entry)
 */
extern inline int pgd_none(pgd_t pgd)		{ return 0; }
extern inline int pgd_bad(pgd_t pgd)		{ return 0; }
extern inline int pgd_present(pgd_t pgd)	{ return 1; }
#define pgd_clear(xp)				do { } while (0)

/*
 * Certain architectures need to do special things when PTEs
 * within a page table are directly modified.  Thus, the following
 * hook is made available.
 */
#define set_pte(pteptr, pteval) (*(pteptr) = pteval) /* set_pte 用来设置页面表项，如果p标志位为1,说明页面不在内存中，发生缺页异常 */
/*
 * (pmds are folded into pgds so this doesnt get actually called,
 * but the define is needed for a generic inline function.)
 */
#define set_pmd(pmdptr, pmdval) (*(pmdptr) = pmdval)
#define set_pgd(pgdptr, pgdval) (*(pgdptr) = pgdval)

#define pgd_page(pgd) \
((unsigned long) __va(pgd_val(pgd) & PAGE_MASK))

extern inline pmd_t * pmd_offset(pgd_t * dir, unsigned long address)
{
	return (pmd_t *) dir;
}
#define ptep_get_and_clear(xp)	__pte(xchg(&(xp)->pte_low, 0))
#define pte_same(a, b)		((a).pte_low == (b).pte_low)
#define pte_page(x)		(mem_map+((unsigned long)(((x).pte_low >> PAGE_SHIFT))))
#define pte_none(x)		(!(x).pte_low)
#define __mk_pte(page_nr,pgprot) __pte(((page_nr) << PAGE_SHIFT) | pgprot_val(pgprot))

#endif /* _I386_PGTABLE_2LEVEL_H */
