#include "bk-hw.h"
#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "qapi/error.h"


#define TYPE_BK_SYSREGS    "bk-sysregs"
#define BK_SYSREGS(obj) OBJECT_CHECK(BkSysregsState, (obj), TYPE_BK_SYSREGS)


typedef struct {
    SysBusDevice sb_dev;
    MemoryRegion sysregs;
    uint16_t buf[128];
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

static uint64_t readfn(void *dev, hwaddr addr, unsigned int size)
{
    BkSysregsState *s = BK_SYSREGS(dev);

    if (addr > SYSREGS_SIZE) {
        printf("SYSREGS: Reading from incorrect address (0x%lx)\n", addr);
    }

    switch (addr) {
        case 060:   // keyboard state
//             printf("kb state\n");
            break;
//         default:
//             printf("SYSREGS: Reading from unknown address (0x%lx)\n", addr);
    }

    printf("SYSREGS: Read from 0x%x %d bytes\n", (unsigned int)addr, size); //TEST

    return s->buf[addr];
}

static void writefn(void *dev, hwaddr addr, uint64_t value,
                        unsigned int size)
{
    BkSysregsState *s = BK_SYSREGS(dev);

    if (addr > SYSREGS_SIZE) {
        printf("SYSREGS: Writing to incorrect address (0x%lx)\n", addr);
    }

    switch (addr) {
        case 060:   // keyboard state
            printf("kb state\n");
            break;
//         default:
//             printf("SYSREGS: Writing to unknown address (0x%lx)\n", addr);
    }
    s->buf[addr] = value;
    printf("SYSREGS: Write to 0x%x %d bytes (%lx)\n", (unsigned int)addr, size, value); //TEST
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
    memory_region_init_ram(&s->sysregs, OBJECT(dev), SYSREGS_NAME, SYSREGS_SIZE, &error_fatal);

    memory_region_init_io(&s->sysregs, OBJECT(dev), &ops, s, SYSREGS_NAME, SYSREGS_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->sysregs);

    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, SYSREGS_BASE);
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
