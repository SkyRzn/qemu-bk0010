#include "bk-hw.h"
#include "qemu/osdep.h"
#include "exec/address-spaces.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "ui/console.h"
#include "sysemu/sysemu.h"
#include "migration/vmstate.h"
#include "cpu.h"
#include "qapi/error.h"
#include <pthread.h>


#define TYPE_BK_KEYBOARD    "bk-keyboard"
#define BK_KEYBOARD(obj) OBJECT_CHECK(BkKeyboardState, (obj), TYPE_BK_KEYBOARD)

#define IRQ_DISBLED_MASK    1 << 6
#define HAS_DATA_MASK       1 << 7

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static CPUState *cpu;

typedef struct {
    SysBusDevice sb_dev;
    MemoryRegion sysregs;
    uint16_t state_reg;
    uint16_t data_reg;
    uint16_t shift;
} BkKeyboardState;


static const VMStateDescription bk_keyboard_vmstate = {
    .name = TYPE_BK_KEYBOARD,
};


void bk_keyboard_init(void)
{
    DeviceState *dev;
    dev = qdev_create(NULL, TYPE_BK_KEYBOARD);
    qdev_init_nofail(dev);

    cpu = qemu_get_cpu(0);
}

static uint64_t kbd_readfn(void *dev, hwaddr addr, unsigned int size)
{
    BkKeyboardState *s = BK_KEYBOARD(dev);
    int res = 0;

    pthread_mutex_lock(&mutex);
    switch (addr) {
        case 060:
            if (size == 2)
                res = s->state_reg;
            else
                res = (s->state_reg >> 8) & 0xff;
            break;
        case 061:
            if (size == 1)
                res = s->state_reg & 0xff;
            break;
        case 062:
            if (size == 2)
                res = s->data_reg;
            else
                res = (s->data_reg >> 8) & 0xff;
            break;
        case 063:
            if (size == 1)
                res = s->data_reg & 0xff;
            break;
    }
    pthread_mutex_unlock(&mutex);
    printf("!!! SYSREGS READ addr=0x%lx val=0x%x\n", addr, res);
    return res;
}

static void kbd_writefn(void *dev, hwaddr addr, uint64_t value,
                        unsigned int size)
{
    BkKeyboardState *s = BK_KEYBOARD(dev);

    pthread_mutex_lock(&mutex);
    switch (addr) {
        case 060:
            if (size == 2)
                s->state_reg = value;
            else
                s->state_reg = (s->state_reg & 0xff) | ((value << 8) & 0xff00);
            break;
        case 061:
            if (size == 1)
                s->state_reg = (s->state_reg & 0xff00) | (value & 0xff);
        case 062:
            printf("!!! KB WRITE DATA REG\n");
            break;
    }
    pthread_mutex_unlock(&mutex);
    printf("!!! SYSREGS WRITE addr=0x%lx val=0x%lx\n", addr, value);
}

static const MemoryRegionOps ops = {
    .read = kbd_readfn,
    .write = kbd_writefn,
    .valid.min_access_size = 1,
    .valid.max_access_size = 2,
//     .endianness = DEVICE_NATIVE_ENDIAN,
};

static void bk_keyboard_event(void *dev, int ch)
{
    BkKeyboardState *s = BK_KEYBOARD(dev);
    int irq_dis;

    if (s->state_reg & HAS_DATA_MASK)
        return;

    pthread_mutex_lock(&mutex);
    s->data_reg = ch;
    s->state_reg |= HAS_DATA_MASK;
    irq_dis = s->state_reg & IRQ_DISBLED_MASK;
    pthread_mutex_unlock(&mutex);

    printf("IRQ dis=%d\n", irq_dis);
    if (!irq_dis)
        cpu_interrupt(cpu, CPU_INTERRUPT_HARD | CPU_INTERRUPT_KEYBOARD);
}

static void bk_keyboard_reset(DeviceState *dev)
{
    BkKeyboardState *s = BK_KEYBOARD(dev);
    s->state_reg = HAS_DATA_MASK;
    s->data_reg = 0;
    s->shift = 0;
}

static void bk_keyboard_realize(DeviceState *dev, Error **err)
{
    BkKeyboardState *s = BK_KEYBOARD(dev);

    memory_region_init_ram(&s->sysregs, OBJECT(dev), TYPE_BK_KEYBOARD, SYSREGS_SIZE, &error_fatal);

    memory_region_init_io(&s->sysregs, OBJECT(dev), &ops, s, TYPE_BK_KEYBOARD, SYSREGS_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->sysregs);

    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, SYSREGS_BASE);

    qemu_add_kbd_event_handler(bk_keyboard_event, s);
}

static void bk_keyboard_class_init(ObjectClass *class, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    dc->vmsd = &bk_keyboard_vmstate;
    dc->realize = bk_keyboard_realize;
    dc->reset = bk_keyboard_reset;
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static const TypeInfo bk_keyboard_info = {
    .name          = TYPE_BK_KEYBOARD,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BkKeyboardState),
    .class_init    = bk_keyboard_class_init,
};

static void bk_keyboard_register_types(void)
{
    type_register_static(&bk_keyboard_info);
}

type_init(bk_keyboard_register_types)
