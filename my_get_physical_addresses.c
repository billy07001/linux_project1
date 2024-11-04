#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>

static void get_pgtable_macro(void)
{
	printk("PAGE_OFFSET = 0x%lx\n", PAGE_OFFSET);
	printk("PGDIR_SHIFT = %d\n", PGDIR_SHIFT);
	printk("P4D_SHIFT = %d\n", P4D_SHIFT);
	printk("PUD_SHIFT = %d\n", PUD_SHIFT);
	printk("PMD_SHIFT = %d\n", PMD_SHIFT);
	printk("PAGE_SHIFT = %d\n", PAGE_SHIFT);

	printk("PTRS_PER_PGD = %d\n", PTRS_PER_PGD);
	printk("PTRS_PER_PUD = %d\n", PTRS_PER_PUD);
	printk("PTRS_PER_PMD = %d\n", PTRS_PER_PMD);
	printk("PTRS_PER_PTE = %d\n", PTRS_PER_PTE);

	printk("PAGE_MASK = 0x%lx\n", PAGE_MASK);
}

static unsigned long vaddr2paddr(unsigned long vaddr) {
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct page *page;
    unsigned long paddr = 0;

    // 取得當前處理程序的 pgd (page global directory)
    pgd = pgd_offset(current->mm, vaddr);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        return 0;

    // 取得 p4d (page 4th-level directory)
    p4d = p4d_offset(pgd, vaddr);
    if (p4d_none(*p4d) || p4d_bad(*p4d))
        return 0;

    // 取得 pud (page upper directory)
    pud = pud_offset(p4d, vaddr);
    if (pud_none(*pud) || pud_bad(*pud))
        return 0;

    // 取得 pmd (page middle directory)
    pmd = pmd_offset(pud, vaddr);
    if (pmd_none(*pmd) || pmd_bad(*pmd))
        return 0;

    // 取得 pte (page table entry)
    pte = pte_offset_map(pmd, vaddr);
    if (!pte || pte_none(*pte))
        return 0;

    // 取得實體頁面
    page = pte_page(*pte);
    if (!page)
        return 0;

    // 計算實體位址
    paddr = (page_to_pfn(page) << PAGE_SHIFT) | (vaddr & ~PAGE_MASK);

    // 釋放 pte 的映射
    pte_unmap(pte);

    return paddr;
}


//自行定義的 syscall
//usr_ptr 是從 user process 直接用指標取得的 vitural address
//unsigned long __user * 的意思是他後面的變數(也就是usr_ptr)是來自user space的
SYSCALL_DEFINE1(my_get_physical_addresses, unsigned long __user *, usr_ptr)
{
	unsigned long paddr;
	unsigned long vaddr;
	get_pgtable_macro();

	if (copy_from_user(&vaddr, usr_ptr, sizeof(unsigned long))) {
        return -EINVAL;
    }

	// 轉換地址
	paddr = vaddr2paddr((unsigned long)usr_ptr);
	if (paddr == (unsigned long)-1) {
		// 如果轉換失敗，返回一個錯誤碼
		return -EINVAL;
	}

	// 如果一切都成功，返回 0
	return paddr;
}