#include "bk-hw.h"
#include "sysregs.h"
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "ui/console.h"
#include "hw/hw.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/display/framebuffer.h"
#include "ui/pixel_ops.h"


typedef struct {
    SysBusDevice parent_obj;
    DisplaySurface *ds;
    QemuConsole *con;
    MemoryRegion sysregs;
    MemoryRegion vram;
    MemoryRegionSection vram_section;
    int invalidate;
} BKDisplayState;

typedef struct {
    uint8_t r: 1;           // reserved
    uint8_t extended: 1;    // extended RAM mode
} DisplayModeBits;

typedef union {
    uint8_t byte;
    DisplayModeBits flags;
} DisplayMode;

typedef struct {
    DisplayMode mode;
    uint8_t offset;
} DisplayRegister;

#define TYPE_BK_DISPLAY "bk-display"
#define TYPE_BK_DISPLAY_VRAM TYPE_BK_DISPLAY "-vram"
#define TYPE_BK_DISPLAY_SYSREGS TYPE_BK_DISPLAY "-sysregs"
#define BK_DISPLAY(obj) OBJECT_CHECK(BKDisplayState, (obj), TYPE_BK_DISPLAY)

#define DEFAULT_OFFSET  0330

#define WIDTH   512
#define HEIGHT  256


static DisplayRegister display_register;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void bk_display_init(void)
{
    DeviceState *dev;
    dev = qdev_create(NULL, TYPE_BK_DISPLAY);
    qdev_init_nofail(dev);
}

uint64_t bk_display_sysregs_readfn(hwaddr addr, unsigned int size)
{
    uint64_t res = 0;
    pthread_mutex_lock(&mutex);
    if (addr == 064) {
        if (size == 1)
            res = display_register.mode.byte;
        else if (size == 2)
            res = display_register.offset + (display_register.mode.byte << 8);
    } else if (addr == 065 && size == 1)
        res = display_register.offset;
    printf("!READ addr=0x%lx, val=0x%lx\n", addr, res);
    pthread_mutex_unlock(&mutex);
    return res;
}

void bk_display_sysregs_writefn(hwaddr addr, uint64_t value, unsigned int size)
{
    pthread_mutex_lock(&mutex);
    if (addr == 064) {
        if (size == 1)
            display_register.mode.byte = value & 0xff;
        else {
            display_register.offset = value & 0xff;
            display_register.mode.byte = (value & 0xff00) >> 8;
        }
    } else if (addr == 065 && size == 1)
        display_register.offset = value &0xff;
    printf("!WRITE addr=0x%lx, val=0x%lx\n", addr, value);
    pthread_mutex_unlock(&mutex);
}

static void bk_display_draw_line(void *dev, uint8_t *d, const uint8_t *s,
                             int width, int pitch)
{
    uint32_t *buf = (uint32_t *)d;
    uint8_t src;
    int i, j;
//     DisplayRegister reg;

    pthread_mutex_lock(&mutex);
//     reg = display_register;
    pthread_mutex_unlock(&mutex);

    for (i = 0; i < WIDTH/8; i++) {
        src = s[i];
        for (j = 0; j < 8; j++) {
            buf[i*8+j] = (src & 1) ? 0xffffffff : 0;
            src >>= 1;
        }
    }
}

static void bk_display_update(void *dev)
{
    BKDisplayState *s = BK_DISPLAY(dev);
    DisplaySurface *surface = qemu_console_surface(s->con);
    int first = 0, last = 0;

    if (s->invalidate) {
        framebuffer_update_memory_section(&s->vram_section, &s->vram, 0, HEIGHT, WIDTH/8);
        s->invalidate = 0;
    }

    framebuffer_update_display(surface, &s->vram_section, WIDTH, HEIGHT,
                               WIDTH/8, WIDTH*4, 0, 1, bk_display_draw_line,
                               s, &first, &last);
    dpy_gfx_update(s->con, 0, 0, WIDTH, HEIGHT);
}

static void bk_display_invalidate(void *dev)
{
    BKDisplayState *s = BK_DISPLAY(dev);
    s->invalidate = 1;
}

// static uint64_t readfn(void *dev, hwaddr addr, unsigned int size)
// {
// //     BKDisplayState *s = BK_DISPLAY(dev);
//     int res = 0;
//
//     printf("!!!!!!!!!!!!!!!!!!!!!! addr=0x%lx\n", addr);
//     pthread_mutex_lock(&mutex);
//     pthread_mutex_unlock(&mutex);
//     return res;
// }
//
// static void writefn(void *dev, hwaddr addr, uint64_t value, unsigned int size)
// {
// //     BKDisplayState *s = BK_DISPLAY(dev);
//
//     printf("!!!!!!!!!!!!!!!!!!!!!! addr=0x%lx val=0x%lx\n", addr, value);
//     pthread_mutex_lock(&mutex);
//     pthread_mutex_unlock(&mutex);
// }
//
// static const MemoryRegionOps sysregs_ops = {
//     .read = readfn,
//     .write = writefn
// };

static const GraphicHwOps graphic_ops = {
    .invalidate  = bk_display_invalidate,
    .gfx_update = bk_display_update,
};

static void bk_display_realize(DeviceState *dev, Error **errp)
{
    BKDisplayState *s = BK_DISPLAY(dev);

    memory_region_init_ram(&s->vram, OBJECT(dev), TYPE_BK_DISPLAY_VRAM, VIDEO_BASE, &error_fatal);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->vram);
    s->invalidate = 1;
    s->con = graphic_console_init(dev, 0, &graphic_ops, s);
    qemu_console_resize(s->con, WIDTH, HEIGHT);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, VIDEO_BASE);

//     bk_sysregs_init_region(dev, TYPE_BK_DISPLAY_SYSREGS , &s->sysregs, &sysregs_ops, SYSREGS_BASE + 064, 2);

}

static void bk_display_class_init(ObjectClass *class, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    dc->realize = bk_display_realize;
    set_bit(DEVICE_CATEGORY_DISPLAY, dc->categories);
    display_register.offset = 0330;
    display_register.mode.byte = 0;
}

static const TypeInfo bk_display_type_info = {
    .name           = TYPE_BK_DISPLAY,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(BKDisplayState),
    .class_init     = bk_display_class_init,
};

static void bk_display_register_types(void)
{
    type_register_static(&bk_display_type_info);
}

type_init(bk_display_register_types)

