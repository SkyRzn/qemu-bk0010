/*
 * QEMU/bk-0010 emulation
 *
 * Copyright (c) 2019 Alexandr Ivanov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "display.h"
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "cpu.h"
#include "hw/hw.h"
#include "sysemu/sysemu.h"
#include "sysemu/qtest.h"
#include "ui/console.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "qemu/error-report.h"
#include "exec/address-spaces.h"
#include "include/hw/sysbus.h"


#define RAM_BASE                0000000
#define RAM_SIZE                0040000

#define MONITOR_ROM_BASE        0100000
#define MONITOR_ROM_SIZE        0120000 - MONITOR_ROM_BASE
#define MONITOR_ROM_FILENAME    "MONIT10.ROM"

#define BASIC_ROM_BASE          0120000
#define BASIC_ROM_SIZE          0200000 - BASIC_ROM_BASE
#define BASIC_ROM_FILENAME      "BASIC10.ROM"


// static uint64_t readfn(void *dev, hwaddr addr, unsigned size)
// {
//     printf("Read from %x\n", (int)addr);
//     return 0;
// }
//
// static void writefn(void *dev, hwaddr addr, uint64_t value,
//                         unsigned size)
// {
//     printf("Write %x to %x\n", (int)value, (int)addr);
// }
//
// static const MemoryRegionOps display_ops = {
//     .read = readfn,
//     .write = writefn,
//     .valid.min_access_size = 1,
//     .valid.max_access_size = 2,
//     .endianness = DEVICE_NATIVE_ENDIAN,
// };


static void bk0010_init(MachineState *machine)
{
    MemoryRegion *address_space, *ram;
    MemoryRegion *monitor_rom, *basic_rom;
    K1801VM1CPU *cpu ATTRIBUTE_UNUSED;
    const char *firmware = NULL;
    const char *filename;

    cpu = K1801VM1_CPU(cpu_create(machine->cpu_type));

    address_space = get_system_memory();

    ram = g_new(MemoryRegion, 1);
    memory_region_allocate_system_memory(ram, NULL, "bk0010.ram", RAM_SIZE);
    memory_region_add_subregion(address_space, RAM_BASE, ram);

    monitor_rom = g_new(MemoryRegion, 1);
    memory_region_allocate_system_memory(monitor_rom, NULL, "bk0010-monitor.rom", MONITOR_ROM_SIZE);
    memory_region_add_subregion(address_space, MONITOR_ROM_BASE, monitor_rom);

    basic_rom = g_new(MemoryRegion, 1);
    memory_region_allocate_system_memory(basic_rom, NULL, "bk0010-basic.rom", BASIC_ROM_SIZE);
    memory_region_add_subregion(address_space, BASIC_ROM_BASE, basic_rom);

    bk_display_init();

    if (machine->firmware)
        firmware = machine->firmware;

    filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, firmware);
    if (!filename)
        filename = MONITOR_ROM_FILENAME;

    if (load_image_targphys(filename, MONITOR_ROM_BASE, MONITOR_ROM_SIZE) < 0) {
        fprintf(stderr, "Error Monitor ROM loading: %s\n", filename);
        exit(-1);
    }

    filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, firmware);
    if (!filename)
        filename = BASIC_ROM_FILENAME;

    if (load_image_targphys(filename, BASIC_ROM_BASE, BASIC_ROM_SIZE) < 0) {
        fprintf(stderr, "Error Basic ROM loading: %s\n", filename);
        exit(-1);
    }
}

static void bk0010_machine_init(MachineClass *mc)
{
    mc->desc = "BK-0010 board";
    mc->init = bk0010_init;
    mc->is_default = 1;
    mc->default_cpu_type = "k1801vm1";
}

DEFINE_MACHINE("bk0010", bk0010_machine_init)
