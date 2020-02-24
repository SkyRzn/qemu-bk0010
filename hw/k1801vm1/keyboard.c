#include "bk-hw.h"
#include "sysregs.h"
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

const char KBD_TABLE[] = {
//        0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
/*00*/    0000, 0014, 0061, 0062, 0063, 0064, 0065, 0066, 0067, 0070, 0071, 0060, 0055, 0075, 0030, 0000,
/*10*/    0161, 0167, 0145, 0162, 0164, 0171, 0165, 0151, 0157, 0160, 0133, 0135, 0012, 0000, 0141, 0163,
/*20*/    0144, 0146, 0147, 0150, 0152, 0153, 0154, 0073, 0047, 0000, 0000, 0134, 0172, 0170, 0143, 0166,
/*30*/    0142, 0156, 0155, 0054, 0056, 0057, 0000, 0000, 0017, 0040, 0000, 0000, 0000, 0000, 0000, 0000,
/*40*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0010, 0000, 0031, 0000, 0000,
/*50*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*60*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*70*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*80*/    0000, 0000, 0041, 0100, 0043, 0044, 0045, 0136, 0046, 0052, 0050, 0051, 0137, 0053, 0000, 0000,
/*90*/    0121, 0127, 0105, 0122, 0124, 0131, 0125, 0111, 0117, 0120, 0173, 0175, 0000, 0000, 0101, 0123,
/*a0*/    0104, 0106, 0107, 0110, 0112, 0113, 0114, 0072, 0042, 0000, 0000, 0174, 0132, 0130, 0103, 0126,
/*b0*/    0102, 0116, 0115, 0074, 0076, 0077, 0000, 0000, 0016, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*c0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*d0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*e0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
/*f0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
};

#define MOD_SHIFT   0x01
#define MOD_E0      0x02
#define MOD_RUS     0x04

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static CPUState *cpu;

typedef struct {
    SysBusDevice sb_dev;
    MemoryRegion sysregs;
    uint16_t state_reg;
    uint16_t data_reg;
    uint16_t mod;
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

static uint64_t readfn(void *dev, hwaddr addr, unsigned int size)
{
    BkKeyboardState *s = BK_KEYBOARD(dev);
    int res = 0;

    pthread_mutex_lock(&mutex);
    if (addr < 4)
        s->state_reg &= ~HAS_DATA_MASK;

    switch (addr) {
        case 0:
            if (size == 2)
                res = s->state_reg;
            else
                res = s->state_reg & 0xff;
            break;
        case 1:
            if (size == 1)
                res = (s->state_reg >> 8) & 0xff;
            break;
        case 2:
            if (size == 2)
                res = s->data_reg;
            else
                res = s->data_reg & 0xff;
            break;
        case 3:
            if (size == 1)
                res = (s->data_reg >> 8) & 0xff;
            break;
    }
//     printf("KBD READ addr=0x%lx size=%d reg=0x%x val=0x%x\n", addr, size, s->data_reg, res);
    pthread_mutex_unlock(&mutex);
    return res;
}

static void writefn(void *dev, hwaddr addr, uint64_t value, unsigned int size)
{
    BkKeyboardState *s = BK_KEYBOARD(dev);

    pthread_mutex_lock(&mutex);
    switch (addr) {
        case 0:
            if (size == 2)
                s->state_reg = value;
            else
                s->state_reg = (s->state_reg & 0xff) | ((value << 8) & 0xff00);
            break;
        case 1:
            if (size == 1)
                s->state_reg = (s->state_reg & 0xff00) | (value & 0xff);
        case 2:
            printf("!!! KB WRITE DATA REG\n");
            break;
    }
//     printf("KBD WRITE addr=0x%lx val=0x%lx\n", addr, value);
    pthread_mutex_unlock(&mutex);
}

static const MemoryRegionOps ops = {
    .read = readfn,
    .write = writefn
};

static void bk_keyboard_event(void *dev, int code)
{
    BkKeyboardState *s = BK_KEYBOARD(dev);
    int irq_dis, mod_e0 = 0;

    switch (code) {
        case 0x2a:
        case 0x36:
            s->mod |= MOD_SHIFT;
            return;
        case 0xaa:
        case 0xb6:
            s->mod &= ~MOD_SHIFT;
            return;
        case 0xe0:
            s->mod |= MOD_E0;
            return;
        case 0x1c:
            if (s->mod & MOD_E0)
                s->mod &= ~MOD_E0;
            break;
        case 0x38: // LAT
            s->mod &= ~MOD_RUS;
            break;
        case 0xb8: // RUS
            s->mod |= MOD_RUS;
            break;
    }

    if (s->mod & MOD_E0) {
        s->mod &= ~MOD_E0;
        mod_e0 = 1;
    }

    if (s->state_reg & HAS_DATA_MASK)
        return;

    if (code > 0x80)
        return;

    if (s->mod & MOD_SHIFT)
        code += 0x80;

    if (mod_e0)
        code += 0x80;        // TEST

    if (code < 0 || code > sizeof(KBD_TABLE) || KBD_TABLE[code] == 0) {
        printf("KBD: Unimplemented keycode 0x%x\n", code);
        return;
    }

//     printf("=== %x\n", code);
    code = KBD_TABLE[code];

    if (code == 0)
        return;

    pthread_mutex_lock(&mutex);
    s->data_reg = code;
    s->state_reg |= HAS_DATA_MASK;
    irq_dis = s->state_reg & IRQ_DISBLED_MASK;
    pthread_mutex_unlock(&mutex);

    if (!irq_dis)
        cpu_interrupt(cpu, CPU_INTERRUPT_HARD | CPU_INTERRUPT_KEYBOARD);
}

static void bk_keyboard_reset(DeviceState *dev)
{
    BkKeyboardState *s = BK_KEYBOARD(dev);
    s->state_reg = HAS_DATA_MASK;
    s->data_reg = 0;
    s->mod = 0;
}

static void bk_keyboard_realize(DeviceState *dev, Error **err)
{
    BkKeyboardState *s = BK_KEYBOARD(dev);
    bk_sysregs_init_region(dev, TYPE_BK_KEYBOARD, &s->sysregs, &ops, SYSREGS_BASE + 060, 4);
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
