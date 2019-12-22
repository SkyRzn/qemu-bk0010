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

/*
 * QEMU AVR CPU
 *
 * Copyright (c) 2016 Michael Rolnik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

/*
 *  NOTE:
 *      This is not a real AVR board !!! This is an example !!!
 *
 *        This example can be used to build a real AVR board.
 *
 *      This example board loads provided binary file into flash memory and
 *      executes it from 0x00000000 address in the code memory space.
 *
 *      Currently used for AVR CPU validation
 *
 */

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


#define RAM_BASE   00000
#define RAM_SIZE   0100000

#define ROM_BASE   0100000
#define ROM_SIZE   0100000

#define ROM_DEFAULT_FILENAME    "monit10.rom"


static void bk0010_init(MachineState *machine)
{
    MemoryRegion *address_space, *ram, *rom;
    K1801VM1CPU *cpu ATTRIBUTE_UNUSED;
    const char *firmware = NULL;
    const char *filename;

    cpu = K1801VM1_CPU(cpu_create(machine->cpu_type));

    address_space = get_system_memory();

    ram = g_new(MemoryRegion, 1);
    memory_region_allocate_system_memory(ram, NULL, "bk0010.ram", RAM_SIZE);
    memory_region_add_subregion(address_space, RAM_BASE, ram);

    rom = g_new(MemoryRegion, 1);
    memory_region_allocate_system_memory(rom, NULL, "bk0010.rom", ROM_SIZE);
    memory_region_add_subregion(address_space, ROM_BASE, rom);

    if (machine->firmware)
        firmware = machine->firmware;

    filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, firmware);
    if (!filename)
        filename = ROM_DEFAULT_FILENAME;

    if (load_image_targphys(filename, ROM_BASE, ROM_SIZE) < 0) {
        fprintf(stderr, "Error ROM loading: %s\n", filename);
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
