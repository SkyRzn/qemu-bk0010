#include "qemu/osdep.h"

#include "cpu.h"

#include "exec/exec-all.h"
#include "exec/cpu_ldst.h"


bool k1801vm1_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                           MMUAccessType access_type, int mmu_idx,
                           bool probe, uintptr_t retaddr)
{
    address &= TARGET_PAGE_MASK;
    tlb_set_page(cs, address, address, PAGE_READ | PAGE_WRITE | PAGE_EXEC, mmu_idx, TARGET_PAGE_SIZE);
    return true;
}

hwaddr k1801vm1_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
    return addr;
}

void k1801vm1_cpu_do_interrupt(CPUState *cs)
{
    printf("IIIIIINT!!!\n");
    switch (cs->exception_index) {
    case K1801VM1_EX_BREAK:
        break;
    default:
        break;
    }
}
