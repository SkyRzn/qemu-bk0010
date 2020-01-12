/*
 *  K1801VM1 emulation
 *
 *  Copyright (c) 2019 Alexandr Ivanov
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _K1801VM1_CPU_H_
#define _K1801VM1_CPU_H_


#include "exec/cpu-defs.h"

#define K1801VM1_REG_NUM        8

// TODO change exception codes
#define K1801VM1_EX_DIV0        0
#define K1801VM1_EX_BAD         1
#define K1801VM1_EX_IRQ         2
#define K1801VM1_EX_SWI         3
#define K1801VM1_EX_MMU_MISS    4
#define K1801VM1_EX_BREAK      16
// TODO change exception codes


struct PswBits {
    uint8_t c: 1;       // carry
    uint8_t v: 1;       // overflow
    uint8_t z: 1;       // zero
    uint8_t n: 1;       // negative
    uint8_t t: 1;       // enable trace
    uint8_t r1: 2;      // reserved
    uint8_t p: 1;       // mask interrupts

    uint8_t c_ch: 1;    // c operation
    uint8_t v_ch: 1;    // v operation
    uint8_t z_ch: 1;    // z changed
    uint8_t n_ch: 1;    // n changed
};

union Psw{                  /* processor status word */
    uint32_t word;
    struct PswBits bits;
};

typedef struct CPUK1801VM1State {
    uint32_t regs[8];

    union Psw psw;

    uint32_t cc_c;  // res
    uint32_t cc_v;  // res ^ ~(arg1 ^ arg2)
    uint32_t cc_zn; // res
//     void *irq[8];

    /* Fields up to this point are cleared by a CPU reset */
    struct {} end_reset_fields;
} CPUK1801VM1State;

#include "hw/core/cpu.h"

#define TYPE_K1801VM1_CPU "k1801vm1"

#define K1801VM1_CPU_CLASS(klass) \
                    OBJECT_CLASS_CHECK(K1801VM1CPUClass, (klass), TYPE_K1801VM1_CPU)
#define K1801VM1_CPU(obj) \
                    OBJECT_CHECK(K1801VM1CPU, (obj), TYPE_K1801VM1_CPU)
#define K1801VM1_CPU_GET_CLASS(obj) \
                    OBJECT_GET_CLASS(K1801VM1CPUClass, (obj), TYPE_K1801VM1_CPU)

typedef struct K1801VM1CPUClass {
    CPUClass parent_class;

    DeviceRealize parent_realize;
    void (*parent_reset)(CPUState *cpu);
} K1801VM1CPUClass;

typedef struct K1801VM1CPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNegativeOffsetState neg;
    CPUK1801VM1State env;
} K1801VM1CPU;

#define K1801VM1_CPU_TYPE_SUFFIX "-" TYPE_K1801VM1_CPU
#define K1801VM1_CPU_TYPE_NAME(model) model K1801VM1_CPU_TYPE_SUFFIX
#define CPU_RESOLVING_TYPE TYPE_K1801VM1_CPU

static inline int cpu_mmu_index(CPUK1801VM1State *env, bool ifetch)
{
    return 0;
}

typedef CPUK1801VM1State CPUArchState;
typedef K1801VM1CPU ArchCPU;

#include "exec/cpu-all.h"

static inline K1801VM1CPU *k1801vm1_env_get_cpu(CPUK1801VM1State *env)
{
    return container_of(env, K1801VM1CPU, env);
}

#define ENV_GET_CPU(e) CPU(k1801vm1_env_get_cpu(e))
#define ENV_OFFSET offsetof(K1801VM1CPU, env)

static inline void cpu_get_tb_cpu_state(CPUK1801VM1State *env, target_ulong *pc,
                                        target_ulong *cs_base, uint32_t *flags)
{
    *pc = env->regs[7];
    *cs_base = 0;
    *flags = 0;
}

// translate.c
extern void k1801vm1_cpu_dump_state(CPUState *cs, FILE *f, int flags);
extern void k1801vm1_translate_init(void);

// helper.c
void k1801vm1_cpu_do_interrupt(CPUState *cs);
bool k1801vm1_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                           MMUAccessType access_type, int mmu_idx,
                           bool probe, uintptr_t retaddr);
hwaddr k1801vm1_cpu_get_phys_page_debug(CPUState *cs, vaddr addr);

#endif /* _K1801VM1_CPU_H_ */
