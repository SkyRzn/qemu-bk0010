/*
 *  K1801VM1 emulation for qemu: main translation routines.
 *
 *  Copyright (c) 2019 Alexandr Ivanov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qemu/qemu-print.h"

#include "cpu.h"
#include "exec/exec-all.h"
#include "tcg-op.h"

enum {
    BS_NONE = 0,    /* Nothing special (none of the below) */
    BS_STOP = 1,    /* We want to stop translation for any reason */
    BS_BRANCH = 2,  /* A branch condition is reached */
    BS_EXCP = 3,    /* An exception condition is reached */
};

typedef struct DisasContext { // TODO FIXME
    struct TranslationBlock *tb;
    CPUK1801VM1State *env;
    int memidx;
    int bstate;
    uint16_t opcode;
} DisasContext;

#include "exec/gen-icount.h"

void k1801vm1_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    K1801VM1CPU *cpu = K1801VM1_CPU(cs);
    CPUK1801VM1State *env = &cpu->env;
//     int i;
    qemu_fprintf(f, "pc=0x%08x\n", env->pc);
//     qemu_fprintf(f, "$fp=0x%08x $sp=0x%08x $r0=0x%08x $r1=0x%08x\n",
//                  env->gregs[0], env->gregs[1], env->gregs[2], env->gregs[3]);
//     for (i = 4; i < 16; i += 4) {
//         qemu_fprintf(f, "$r%d=0x%08x $r%d=0x%08x $r%d=0x%08x $r%d=0x%08x\n",
//                      i - 2, env->gregs[i], i - 1, env->gregs[i + 1],
//                      i, env->gregs[i + 2], i + 1, env->gregs[i + 3]);
//     }
//     for (i = 4; i < 16; i += 4) {
//         qemu_fprintf(f, "sr%d=0x%08x sr%d=0x%08x sr%d=0x%08x sr%d=0x%08x\n",
//                      i - 2, env->sregs[i], i - 1, env->sregs[i + 1],
//                      i, env->sregs[i + 2], i + 1, env->sregs[i + 3]);
//     }
}

void k1801vm1_translate_init(void)
{
//     int i;
//     static const char * const gregnames[16] = {
//         "$fp", "$sp", "$r0", "$r1",
//         "$r2", "$r3", "$r4", "$r5",
//         "$r6", "$r7", "$r8", "$r9",
//         "$r10", "$r11", "$r12", "$r13"
//     };
//
//     cpu_pc = tcg_global_mem_new_i32(cpu_env, offsetof(CPUMoxieState, pc), "$pc");
//     for (i = 0; i < 16; i++)
//         cpu_gregs[i] = tcg_global_mem_new_i32(cpu_env, offsetof(CPUMoxieState, gregs[i]), gregnames[i]);
//     cc_a = tcg_global_mem_new_i32(cpu_env, offsetof(CPUMoxieState, cc_a), "cc_a");
//     cc_b = tcg_global_mem_new_i32(cpu_env, offsetof(CPUMoxieState, cc_b), "cc_b");
}

void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int max_insns)
{
//     CPUK1801VM1State *env = cs->env_ptr;
//     K1801VM1CPU *cpu = env_archcpu(env);
//     DisasContext ctx;

//     ctx.tb = tb;
//     ctx.memidx = 0;
//     ctx.bstate = BS_NONE;

}

void restore_state_to_opc(CPUK1801VM1State *env, TranslationBlock *tb,
                          target_ulong *data)
{
    env->pc = data[0];
}

