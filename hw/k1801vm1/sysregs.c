#include "bk-hw.h"
#include "sysregs.h"
#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "qapi/error.h"


#define TYPE_BK_SYSREGS    "bk-sysregs"
#define BK_SYSREGS(obj) OBJECT_CHECK(BkSysregsState, (obj), TYPE_BK_SYSREGS)


typedef struct {
    SysBusDevice sb_dev;
    MemoryRegion sysregs;
} BkSysregsState;


static const VMStateDescription bk_sysregs_vmstate = {
    .name = TYPE_BK_SYSREGS,
};


void bk_sysregs_init(void)
{
    DeviceState *dev;
    dev = qdev_create(NULL, TYPE_BK_SYSREGS);
    qdev_init_nofail(dev);
}

void bk_sysregs_init_region(void *dev, const char *type, MemoryRegion *region,
    const MemoryRegionOps *ops, hwaddr base, uint64_t size)
{
//     memory_region_init_ram(region, OBJECT(dev), type, size, &error_fatal);
    memory_region_init_io(region, OBJECT(dev), ops, dev, type, size);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), region);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, base);
}

static uint64_t readfn(void *dev, hwaddr addr, unsigned int size)
{
    switch (addr) {
        case 064:       // TODO display offset
        case 065:       // TODO display offset
            return bk_display_sysregs_readfn(addr, size);
        case 0114:      // IO port
            return 0;
        case 0116:      // system port
            return (size == 1) ? 0x80 : 0x8080;
        default:
            printf("Unimplemented system register 0%o >> (size=%d)\n", 0177600+(unsigned int)addr, size); //TEST
    }
    return 0;
}

static void writefn(void *dev, hwaddr addr, uint64_t value,
                        unsigned int size)
{
    switch (addr) {
        case 064:       // TODO display offset
        case 065:
            bk_display_sysregs_writefn(addr, value, size);
            break;
        case 0114:      // IO port
        case 0116:      // system port
            break;
        default:
            printf("Unimplemented system register 0%o << (val=0x%lx, size=%d)\n", 0177600+(unsigned int)addr, value, size); //TEST
    }
}

static const MemoryRegionOps ops = {
    .read = readfn,
    .write = writefn,
    .valid.min_access_size = 1,
    .valid.max_access_size = 2,
};

static void bk_sysregs_reset(DeviceState *dev)
{
}

static void bk_sysregs_realize(DeviceState *dev, Error **err)
{
    BkSysregsState *s = BK_SYSREGS(dev);
//     memory_region_init_ram(&s->sysregs, OBJECT(dev), SYSREGS_NAME, SYSREGS_SIZE, &error_fatal);
    bk_sysregs_init_region(dev, SYSREGS_NAME, &s->sysregs, &ops, SYSREGS_BASE, SYSREGS_SIZE);
}

static void bk_sysregs_class_init(ObjectClass *class, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    dc->vmsd = &bk_sysregs_vmstate;
    dc->realize = bk_sysregs_realize;
    dc->reset = bk_sysregs_reset;
    set_bit(DEVICE_CATEGORY_CPU, dc->categories);
}

static const TypeInfo bk_sysregs_info = {
    .name          = TYPE_BK_SYSREGS,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BkSysregsState),
    .class_init    = bk_sysregs_class_init,
};

static void bk_sysregs_register_types(void)
{
    type_register_static(&bk_sysregs_info);
}

type_init(bk_sysregs_register_types)
