/*
 * K1801VM1 cpu parameters for qemu.
 *
 * Copyright (c) 2019 Alexandr Ivanov
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef K1801VM1_CPU_PARAM_H
#define K1801VM1_CPU_PARAM_H 1

#define TARGET_LONG_BITS 32
#define TARGET_PAGE_BITS 12     /* 4k */
#define TARGET_PHYS_ADDR_SPACE_BITS 16
#define TARGET_VIRT_ADDR_SPACE_BITS 16
#define NB_MMU_MODES 1

#endif
