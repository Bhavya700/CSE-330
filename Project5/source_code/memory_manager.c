#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pid.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bhavya Patel");

static int pid = -1;
static unsigned long long addr = 0;
module_param(pid, int, 0);
module_param(addr, ullong, 0);

static int __init memory_manager_init(void)
{
    struct task_struct *task;
    struct mm_struct *mm;
    unsigned long address = addr;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    if (pid < 0 || addr == 0) {
        pr_err("[CSE330-Memory-Manager] Module Error Invalid PID or Address\n");
        return 0;
    }

    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        pr_info("[CSE330-Memory-Manager] Module loaded with PID [%d] not found\n", pid);
        return 0;
    }

    mm = task->mm;
    if (!mm) {
        pr_info("[CSE330-Memory-Manager] Module loaded with PID [%d] has no memory space\n", pid);
        return 0;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
    down_read(&mm->mmap_lock);
#else
    down_read(&mm->mmap_sem);
#endif

    pgd = pgd_offset(mm, address);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        goto invalid;

    p4d = p4d_offset(pgd, address);
    if (p4d_none(*p4d) || p4d_bad(*p4d))
        goto invalid;

    pud = pud_offset(p4d, address);
    if (pud_none(*pud) || pud_bad(*pud))
        goto invalid;

    pmd = pmd_offset(pud, address);
    if (pmd_none(*pmd) || pmd_bad(*pmd))
        goto invalid;

    pte = pte_offset_kernel(pmd, address);
    if (!pte)
        goto invalid;

    if (pte_none(*pte))
        goto invalid;

    if (pte_present(*pte)) {
        unsigned long pfn = pte_pfn(*pte);
        unsigned long phys_addr = (pfn << PAGE_SHIFT) | (address & ~PAGE_MASK);
        pr_info("[CSE330-Memory-Manager] Module loaded with PID [%d]: virtual address [0x%llx] physical address [0x%lx] swap identifier [NA]\n",
                pid, addr, phys_addr);
    } else if (is_swap_pte(*pte)) {
        swp_entry_t entry = pte_to_swp_entry(*pte);
        pr_info("[CSE330-Memory-Manager] Module loaded with PID [%d]: virtual address [0x%llx] physical address [NA] swap identifier [0x%lx]\n",
                pid, addr, entry.val);
    } else {
        goto invalid;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
    up_read(&mm->mmap_lock);
#else
    up_read(&mm->mmap_sem);
#endif

    return 0;

invalid:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
    up_read(&mm->mmap_lock);
#else
    up_read(&mm->mmap_sem);
#endif
    pr_info("[CSE330-Memory-Manager] Module loaded with PID [%d]: virtual address [0x%llx] physical address [NA] swap identifier [NA]\n",
            pid, addr);
    return 0;
}

static void __exit memory_manager_exit(void)
{
}

module_init(memory_manager_init);
module_exit(memory_manager_exit);

