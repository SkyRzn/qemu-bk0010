#ifndef _BK_0010_HW_SYSREGS_H_
#define _BK_0010_HW_SYSREGS_H_


#include "qemu/osdep.h"
#include "exec/memory.h"


typedef uint64_t bk_sysregs_readfn(void *dev, hwaddr addr, unsigned int size);
typedef void bk_sysregs_writefn(void *dev, hwaddr addr, uint64_t value, unsigned int size);


extern void bk_sysregs_init_region(void *dev, const char *type, MemoryRegion *region,
                                    const MemoryRegionOps *ops, hwaddr base, uint64_t size);


#endif

