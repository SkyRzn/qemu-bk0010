/*
 * QEMU K1801VM1 CPU
 *
 * Copyright (c) 2019 Alexandr Ivanov
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

#include "qemu/osdep.h"
#include "cpu.h"
#include "machine.h"
#include "migration/cpu.h"


const VMStateDescription vmstate_k1801vm1_cpu = {
    .name = "cpu",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT16(r0, CPUK1801VM1State),
        VMSTATE_UINT16(r1, CPUK1801VM1State),
        VMSTATE_UINT16(r2, CPUK1801VM1State),
        VMSTATE_UINT16(r3, CPUK1801VM1State),
        VMSTATE_UINT16(r4, CPUK1801VM1State),
        VMSTATE_UINT16(r5, CPUK1801VM1State),
        VMSTATE_UINT16(sp, CPUK1801VM1State),
        VMSTATE_UINT16(pc, CPUK1801VM1State),
        VMSTATE_UINT16(psw.word, CPUK1801VM1State),
        VMSTATE_END_OF_LIST()
    }
};
