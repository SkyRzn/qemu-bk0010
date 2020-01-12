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

enum Cc_cmd {
    CC_CMD_ADD,
    CC_CMD_SUB,
    CC_CMD_ASL
};

#define CC_PSW_SHIFT_C      0
#define CC_PSW_SHIFT_V      1
#define CC_PSW_SHIFT_Z      2
#define CC_PSW_SHIFT_N      3
#define CC_PSW_MASK_C       0x00000001
#define CC_PSW_MASK_V       0x00000002
#define CC_PSW_MASK_Z       0x00000004
#define CC_PSW_MASK_N       0x00000008

#define CC_PSW_SHIFT_C_CH   8
#define CC_PSW_SHIFT_V_CH   9
#define CC_PSW_SHIFT_Z_CH   10
#define CC_PSW_SHIFT_N_CH   11
#define CC_PSW_MASK_C_CH    0x00000100
#define CC_PSW_MASK_V_CH    0x00000200
#define CC_PSW_MASK_Z_CH    0x00000400
#define CC_PSW_MASK_N_CH    0x00000800

typedef struct DisasContext { // TODO FIXME
    struct TranslationBlock *tb;
    CPUK1801VM1State *env;
    int memidx;
    int bstate;
    uint16_t opcode;
    target_ulong pc;

} DisasContext;

static TCGv cpu_regs[K1801VM1_REG_NUM];
static TCGv cpu_reg_psw;

static TCGv cpu_cc_c;
static TCGv cpu_cc_v;
static TCGv cpu_cc_zn;

#define cpu_reg_pc cpu_regs[7]
#define cpu_reg_sp cpu_regs[6]

#include "exec/gen-icount.h"

void k1801vm1_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    K1801VM1CPU *cpu = K1801VM1_CPU(cs);
    CPUK1801VM1State *env = &cpu->env;
//     int i;
//     for (i = 0; i < 8; i++) {
//         qemu_fprintf(f, "%d=0x%04x  ", i, (uint16_t)env->regs[i]);
//     }
//     qemu_fprintf(f, "psw=0x%04x  ", (uint16_t)env->psw.word);
//     qemu_fprintf(f, "\n");

    int i;
    qemu_fprintf(f, "%c", (env->psw.bits.n) ? 'N' : ' ');
    qemu_fprintf(f, "%c", (env->psw.bits.z) ? 'Z' : ' ');
    qemu_fprintf(f, "%c", (env->psw.bits.v) ? 'V' : ' ');
    qemu_fprintf(f, "%c", (env->psw.bits.c) ? 'C' : ' ');
    qemu_fprintf(f, "  %04x: ", env->regs[7]);
    for (i = 0; i < 7; i++)
        qemu_fprintf(f, "r%d=0x%04x ", i, env->regs[i]);
//     printf("  [");
//     for (i = 0; i < 8; i++)
//         printf("%d=%06o  ", i, env->regs[i]);
//     printf("]\n");
    qemu_fprintf(f, "\n");

}

void k1801vm1_translate_init(void)
{
    const char *reg_names[K1801VM1_REG_NUM] = {
        "$r0", "$r1", "$r2", "$r3",
        "$r4", "$r5", "$sp", "$pc"
    };
    int i;

    for (i = 0; i < K1801VM1_REG_NUM; i++) {
        cpu_regs[i] = tcg_global_mem_new_i32(cpu_env,
                                             offsetof(CPUK1801VM1State, regs[i]),
                                             reg_names[i]);
    }
    cpu_reg_psw = tcg_global_mem_new_i32(cpu_env, offsetof(CPUK1801VM1State, psw), "$psw");

    cpu_cc_c = tcg_global_mem_new_i32(cpu_env, offsetof(CPUK1801VM1State, cc_c), "cc_c");
    cpu_cc_v = tcg_global_mem_new_i32(cpu_env, offsetof(CPUK1801VM1State, cc_v), "cc_v");
    cpu_cc_zn = tcg_global_mem_new_i32(cpu_env, offsetof(CPUK1801VM1State, cc_zn), "cc_zn");
}

static void print_regs(CPUK1801VM1State *env, DisasContext *ctx)
{
//     int i;
//     printf("%c", (env->psw.bits.n) ? 'N' : ' ');
//     printf("%c", (env->psw.bits.z) ? 'Z' : ' ');
//     printf("%c", (env->psw.bits.v) ? 'V' : ' ');
//     printf("%c", (env->psw.bits.c) ? 'C' : ' ');
//     printf("  %04x: [%06o]", ctx->pc, ctx->opcode);
//     for (i = 0; i < 7; i++)
//         printf("r%d=0x%04x ", i, env->regs[i]);
//     printf("\n");
}

#define loadb(env, ctx, t, addr, change)    load_operand(env, ctx, t, addr, 1, change)
#define loadw(env, ctx, t, addr, change)    load_operand(env, ctx, t, addr, 0, change)
static void load_operand(CPUK1801VM1State *env, DisasContext *ctx, TCGv t, int addr, int byte, int change)
{
    int mode, reg;

    mode = addr & 070;
    reg =  addr & 007;

    if (reg == 7) {
        switch (mode) {
            case 000:
                tcg_gen_mov_tl(t, cpu_regs[reg]);
                break;
            case 020:
                tcg_gen_movi_tl(t, (byte) ? cpu_ldub_code(env, ctx->pc) : cpu_lduw_code(env, ctx->pc));
                break;
            case 030:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_movi_tl(t, 0);
                    tcg_gen_movi_tl(t1, cpu_lduw_code(env, ctx->pc));
                    if (byte)
                        tcg_gen_qemu_ld8s(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16s(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;
            case 060:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    int addr = ctx->pc + cpu_ldsw_code(env, ctx->pc) + 2;
                    addr &= 0xffff; //TEST it works but I need to make it better
                    tcg_gen_movi_tl(t1, addr);
                    if (byte)
                        tcg_gen_qemu_ld8s(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16s(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;
            case 070:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    int addr = ctx->pc + cpu_lduw_code(env, ctx->pc) + 2;
                    tcg_gen_movi_tl(t1, addr); // TODO check if the addr is right
                    tcg_gen_qemu_ld16u(t1, t1, ctx->memidx);
                    if (byte)
                        tcg_gen_qemu_ld8s(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16s(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;

            default:
                printf("INCORRECT LOAD mode=%o, reg=7\n", mode); //TEST
                exit(-1); //TEST
        }
        if (change)
            ctx->pc += 2;
    } else {
        switch (mode) {
            case 000:
                tcg_gen_mov_tl(t, cpu_regs[reg]);
                break;
            case 010:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_andi_tl(t1, cpu_regs[reg], 0xffff);
                    if (byte)
                        tcg_gen_qemu_ld8s(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16s(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;
            case 020:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_andi_tl(t1, cpu_regs[reg], 0xffff);
                    if (byte)
                        tcg_gen_qemu_ld8s(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16s(t, t1, ctx->memidx);
                    if (change)
                        tcg_gen_addi_tl(cpu_regs[reg], t1, (byte) ? 1 : 2);
                    tcg_temp_free_i32(t1);
                }
                break;
            case 030:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    TCGv t2 = tcg_temp_new_i32();
                    tcg_gen_andi_tl(t1, cpu_regs[reg], 0xffff);
                    tcg_gen_qemu_ld16u(t2, t1, ctx->memidx);
                    if (byte)
                        tcg_gen_qemu_ld8s(t, t2, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16s(t, t2, ctx->memidx);
                    if (change) {
                        tcg_gen_ext16s_tl(cpu_regs[reg], t1);
                        tcg_gen_addi_tl(cpu_regs[reg], cpu_regs[reg], (byte) ? 1 : 2);

                    }
                    tcg_temp_free_i32(t1);
                    tcg_temp_free_i32(t2);
                }
                break;
            case 040:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_subi_tl(t1, cpu_regs[reg], (byte) ? 1 : 2);
                    if (change)
                        tcg_gen_mov_tl(cpu_regs[reg], t1);
                    tcg_gen_andi_tl(t1, t1, 0xffff);
                    if (byte)
                        tcg_gen_qemu_ld8s(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16s(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;
            case 060:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_addi_tl(t1, cpu_regs[reg], cpu_lduw_code(env, ctx->pc));
                    tcg_gen_andi_tl(t1, t1, 0xffff);
                    if (byte) {
                        tcg_gen_qemu_ld8s(t, t1, ctx->memidx);
                    } else {
                        tcg_gen_qemu_ld16s(t, t1, ctx->memidx);
                    }
                    tcg_temp_free_i32(t1);
                    if (change)
                        ctx->pc += 2;
                }
                break;
            case 050:
            case 070:
            default:
                printf("!!!!!!!!!!!!! READ SS %o\n", mode);
                exit(-1); //TEST
        }
    }
}

#define storeb(env, ctx, t, addr, change)    store_operand(env, ctx, t, addr, 1, change)
#define storew(env, ctx, t, addr, change)    store_operand(env, ctx, t, addr, 0, change)
static void store_operand(CPUK1801VM1State *env, DisasContext *ctx, TCGv t, int addr, int byte, int change)
{
    int mode, reg;

    mode = addr & 070;
    reg =  addr & 007;

    if (reg == 7) {
        switch (mode) {
            case 000:
                tcg_gen_mov_tl(cpu_regs[reg], t);
                break;
            case 020:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_movi_tl(t1, cpu_lduw_code(env, ctx->pc));
                    if (byte)
                        tcg_gen_qemu_st8(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_st16(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;
            case 030:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_movi_tl(t1, cpu_lduw_code(env, ctx->pc));
                    if (byte)
                        tcg_gen_qemu_st8(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_st16(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;
            case 060:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    int addr = ctx->pc + cpu_lduw_code(env, ctx->pc) + 2;
                    addr &= 0xffff;
                    tcg_gen_movi_tl(t1, addr);
                    if (byte)
                        tcg_gen_qemu_st8(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_st16(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;
            default:
                printf("INCORRECT STORE mode=%o, reg=7\n", mode); //TEST
        }
        if (change)
            ctx->pc += 2;
    } else {
        switch (mode) {
            case 000:
                tcg_gen_mov_tl(cpu_regs[reg], t);
                break;
            case 010:
                if (byte)
                    tcg_gen_qemu_st8(t, cpu_regs[reg], ctx->memidx);
                else
                    tcg_gen_qemu_st16(t, cpu_regs[reg], ctx->memidx);
                break;
            case 020:
                if (byte)
                    tcg_gen_qemu_st8(t, cpu_regs[reg], ctx->memidx);
                else
                    tcg_gen_qemu_st16(t, cpu_regs[reg], ctx->memidx);
                if (change)
                    tcg_gen_addi_tl(cpu_regs[reg], cpu_regs[reg], (byte) ? 1 : 2);
                break;
            case 040:
                tcg_gen_subi_tl(cpu_regs[reg], cpu_regs[reg], (byte) ? 1 : 2);
                if (byte)
                    tcg_gen_qemu_st8(t, cpu_regs[reg], ctx->memidx);
                else
                    tcg_gen_qemu_st16(t, cpu_regs[reg], ctx->memidx);
                if (!change)
                    tcg_gen_addi_tl(cpu_regs[reg], cpu_regs[reg], (byte) ? 1 : 2);
                break;

            case 030:
            case 050:
            case 060:
            case 070:
            default:
                printf("!!!!!!!!!!!!! WRITE DD %o\n", mode);
        }
    }
}

static void load_addr(CPUK1801VM1State *env, DisasContext *ctx, int addr)
{
    int mode, reg;

    mode = addr & 070;
    reg =  addr & 007;

    if (reg == 7) {
        switch (mode) {
            case 030:
                tcg_gen_movi_tl(cpu_reg_pc, cpu_lduw_code(env, ctx->pc));
                break;
            case 060:
                {
                    int addr = ctx->pc + cpu_ldsw_code(env, ctx->pc) + 2;
                    tcg_gen_movi_tl(cpu_reg_pc, addr);
                    tcg_gen_andi_tl(cpu_reg_pc, cpu_reg_pc, 0xffff);
                    ctx->pc += 2;
                }
                break;
            default:
                printf("!!!!!!!!!!!!! JUMP INCORRECT MODE %o (reg=%o)\n", mode, reg);
                exit(-1); //TEST
        }
    } else {
        switch (mode) {
            case 010:
                tcg_gen_mov_tl(cpu_reg_pc, cpu_regs[reg]);
                tcg_gen_andi_tl(cpu_reg_pc, cpu_reg_pc, 0xffff);
                break;
            default:
                printf("!!!!!!!!!!!!! JUMP INCORRECT MODE %o (reg=%o)\n", mode, reg);
                exit(-1); //TEST
        }
    }
}

static void push(DisasContext *ctx, TCGv t)
{
    tcg_gen_subi_tl(cpu_reg_sp, cpu_reg_sp, 2);
    tcg_gen_qemu_st16(t, cpu_reg_sp, ctx->memidx);
}

static void pop(DisasContext *ctx, TCGv t)
{
    tcg_gen_movi_tl(t, 0);
    tcg_gen_qemu_ld16s(t, cpu_reg_sp, ctx->memidx);
    tcg_gen_andi_tl(t, t, 0xffff);           //TODO ERROR ffff805e
    tcg_gen_addi_tl(cpu_reg_sp, cpu_reg_sp, 2);
}

static void cc_set_cv(int c, int v)
{
    if (c == 0)
        tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~CC_PSW_MASK_C);
    else if (c == 1)
        tcg_gen_ori_tl(cpu_reg_psw, cpu_reg_psw, CC_PSW_MASK_C);

    if (v == 0)
        tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~CC_PSW_MASK_V);
    else if (v == 1)
        tcg_gen_ori_tl(cpu_reg_psw, cpu_reg_psw, CC_PSW_MASK_V);
}

static void cc_save_zn(TCGv res, int mask)
{
    tcg_gen_ori_tl(cpu_reg_psw, cpu_reg_psw, mask);
    tcg_gen_mov_tl(cpu_cc_zn, res);
}

static void cc_save_cv(TCGv arg1, TCGv arg2, TCGv res, int mask, enum Cc_cmd cmd)
{
    tcg_gen_ori_tl(cpu_reg_psw, cpu_reg_psw, mask);
    if (mask & CC_PSW_MASK_V_CH) {
        TCGv t = tcg_temp_new_i32();
        if (cmd == CC_CMD_ADD) {
            tcg_gen_xor_tl(cpu_cc_v, arg1, res);
            tcg_gen_xor_tl(t, arg1, arg2);
            tcg_gen_andc_tl(cpu_cc_v, cpu_cc_v, t);
        } else if (cmd == CC_CMD_SUB) {
            tcg_gen_xor_tl(cpu_cc_v, arg1, res);
            tcg_gen_xor_tl(t, arg1, arg2);
            tcg_gen_and_tl(cpu_cc_v, cpu_cc_v, t); // TODO check it
        } else if (cmd == CC_CMD_ASL) {
            tcg_gen_xor_tl(cpu_cc_v, arg1, res);
        }
        tcg_temp_free_i32(t);
    }
    if (mask & CC_PSW_MASK_C_CH) {
        tcg_gen_mov_tl(cpu_cc_c, res);
    }
}

static void cc_gen_psw(DisasContext *ctx)
{
    TCGv t = tcg_temp_new_i32();

    // Z
    TCGLabel *l_zend = gen_new_label();
    tcg_gen_andi_tl(t, cpu_reg_psw, CC_PSW_MASK_Z_CH);
    tcg_gen_brcondi_tl(TCG_COND_EQ, t, 0, l_zend);                 // if Z hasn't been changed goto l_zend
    tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~CC_PSW_MASK_Z_CH);   // reset Z_CH
    tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~CC_PSW_MASK_Z);      // set Z to 0
    tcg_gen_brcondi_tl(TCG_COND_NE, cpu_cc_zn, 0, l_zend);         // if NZ is not zero goto l_zend
    tcg_gen_ori_tl(cpu_reg_psw, cpu_reg_psw, CC_PSW_MASK_Z);        // set Z to 1
    gen_set_label(l_zend);

    // N
    TCGLabel *l_nend = gen_new_label();
    tcg_gen_andi_tl(t, cpu_reg_psw, CC_PSW_MASK_N_CH);
    tcg_gen_brcondi_tl(TCG_COND_EQ, t, 0, l_nend);                 // if N hasn't been changed goto l_nend
    tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~CC_PSW_MASK_N_CH);    // reset N_CH
    tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~CC_PSW_MASK_N);       // set N to 0
    tcg_gen_andi_tl(t, cpu_cc_zn, 0x8000);                          // ZN & SIGN_MASK
    tcg_gen_brcondi_tl(TCG_COND_EQ, t, 0, l_nend);                 // if NZ is not negative goto l_nend
    tcg_gen_ori_tl(cpu_reg_psw, cpu_reg_psw, CC_PSW_MASK_N);       // set N to 1
    gen_set_label(l_nend);

    // C
    TCGLabel *l_cend = gen_new_label();
    tcg_gen_andi_tl(t, cpu_reg_psw, CC_PSW_MASK_C_CH);
    tcg_gen_brcondi_tl(TCG_COND_EQ, t, 0, l_cend);                 // if C hasn't been changed goto l_cend
    tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~CC_PSW_MASK_C_CH);   // reset C_CH
    tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~CC_PSW_MASK_C);      // set C to 0

    tcg_gen_andi_tl(t, cpu_cc_c, 0x10000);                          // C & CARRY_MASK
    tcg_gen_brcondi_tl(TCG_COND_EQ, t, 0, l_cend);                 // if C is 0 goto l_cend
    tcg_gen_ori_tl(cpu_reg_psw, cpu_reg_psw, CC_PSW_MASK_C);       // set C to 1
    gen_set_label(l_cend);

    // V
    TCGLabel *l_vend = gen_new_label();
    tcg_gen_andi_tl(t, cpu_reg_psw, CC_PSW_MASK_V_CH);
    tcg_gen_brcondi_tl(TCG_COND_EQ, t, 0, l_vend);
    tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~CC_PSW_MASK_V_CH);
    tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~CC_PSW_MASK_V);

    tcg_gen_andi_tl(t, cpu_cc_v, 0x8000);
    tcg_gen_brcondi_tl(TCG_COND_EQ, t, 0, l_vend);
    tcg_gen_ori_tl(cpu_reg_psw, cpu_reg_psw, CC_PSW_MASK_V);
    gen_set_label(l_vend);

    tcg_temp_free_i32(t);
}

static inline int decode_dop(CPUK1801VM1State *env, DisasContext *ctx, int op)
{
    int oppart, src, dst, b;

    oppart = op & 0070000;
    if (oppart == 0)
        return 0;

    b   = op & 0100000;
    dst = op & 0000077;

    if (oppart != 0070000) {
        src = (op & 0007700) >> 6;
        switch (oppart) {
            case 0010000: // MOV
                load_operand(env, ctx, cpu_cc_zn, src, b, 1);
                store_operand(env, ctx, cpu_cc_zn, dst, b, 1);
                cc_save_zn(cpu_cc_zn, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                break;
            case 0020000: // CMP
                {
                    TCGv t1 = tcg_temp_new_i32();
                    TCGv t2 = tcg_temp_new_i32();
                    TCGv t3 = tcg_temp_new_i32();
                    load_operand(env, ctx, t1, src, b, 1);
                    load_operand(env, ctx, t2, dst, b, 1);
                    tcg_gen_sub_tl(t3, t1, t2);       // dest - src
                    cc_save_zn(t3, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                    cc_save_cv(t1, t2, t3, CC_PSW_MASK_C_CH | CC_PSW_MASK_V_CH, CC_CMD_SUB);
                    tcg_temp_free_i32(t1);
                    tcg_temp_free_i32(t2);
                    tcg_temp_free_i32(t3);
                }
                break;
            case 0030000: // BIT(B)
                {
                    TCGv t1 = tcg_temp_new_i32();
                    TCGv t2 = tcg_temp_new_i32();
                    load_operand(env, ctx, t1, src, b, 1);
                    load_operand(env, ctx, t2, dst, b, 1);
                    tcg_gen_and_tl(t2, t2, t1);       // dest & src
                    cc_save_zn(t2, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                    cc_set_cv(-1, 0);
                    tcg_temp_free_i32(t1);
                    tcg_temp_free_i32(t2);
                }
                break;
            case 0040000: // BIC
                {
                    TCGv t1 = tcg_temp_new_i32();
                    TCGv t2 = tcg_temp_new_i32();
                    load_operand(env, ctx, t1, src, b, 1);
                    load_operand(env, ctx, t2, dst, b, 1);
                    tcg_gen_andc_tl(t2, t2, t1);       // dest &= ~src
                    tcg_gen_ext16s_tl(t2, t2);
                    store_operand(env, ctx, t2, dst, b, 0);
                    cc_save_zn(t2, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                    cc_set_cv(-1, 0);
                    tcg_temp_free_i32(t1);
                    tcg_temp_free_i32(t2);
                }
                break;
            case 0050000: // BIS
                {
                    TCGv t1 = tcg_temp_new_i32();
                    TCGv t2 = tcg_temp_new_i32();
                    load_operand(env, ctx, t1, src, b, 1);
                    load_operand(env, ctx, t2, dst, b, 1);
                    tcg_gen_or_tl(t2, t2, t1);       // dest |= src
                    store_operand(env, ctx, t2, dst, b, 0);
                    cc_save_zn(t2, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                    cc_set_cv(-1, 0);
                    tcg_temp_free_i32(t1);
                    tcg_temp_free_i32(t2);
                }
                break;
            case 0060000:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    TCGv t2 = tcg_temp_new_i32();
                    loadw(env, ctx, t1, src, 1);
                    loadw(env, ctx, t2, dst, 1);
                    if (b) {                                 // SUB
                        tcg_gen_sub_tl(cpu_cc_zn, t2, t1);
                        tcg_gen_ext16s_tl(cpu_cc_zn, cpu_cc_zn);
                    } else {                                // ADD
                        tcg_gen_add_tl(cpu_cc_zn, t1, t2);
                        tcg_gen_ext16s_tl(cpu_cc_zn, cpu_cc_zn);
                    }
                    cc_save_zn(cpu_cc_zn, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                    cc_save_cv(t1, t2, cpu_cc_zn,
                               CC_PSW_MASK_C_CH | CC_PSW_MASK_V_CH,
                               (b) ? CC_CMD_SUB : CC_CMD_ADD);
                    storew(env, ctx, cpu_cc_zn, dst, 0);
                    tcg_temp_free_i32(t1);
                    tcg_temp_free_i32(t2);
                }
                break;
            default:
                return 0;
        }
    } else {
        oppart = op & 0007000;
        src = (op & 0000700) >> 6;
        switch (oppart) {
//             case 0000000: // MUL
//                 break;
//             case 0001000: // DIV
//                 break;
//             case 0002000: // ASH
//                 break;
//             case 0003000: // ASHC
//                 break;
            case 0004000: // XOR
                {
                    TCGv t = tcg_temp_new_i32();
                    loadw(env, ctx, cpu_cc_zn, src, 1);
                    loadw(env, ctx, t, dst, 1);
                    tcg_gen_xor_tl(cpu_cc_zn, cpu_cc_zn, t);
                    storew(env, ctx, cpu_cc_zn, dst, 0);
                    cc_save_zn(cpu_cc_zn, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                    cc_set_cv(-1, 0);
                    tcg_temp_free_i32(t);
                }
                break;
//             case 0005000: // floating point ops
//                 break;
//             case 0006000: // system instructions
//                 break;
            case 0007000:                   // SOB    Subtract one and branch
                {
                    tcg_gen_subi_tl(cpu_regs[src], cpu_regs[src], 1);
                    tcg_gen_ext16s_tl(cpu_regs[src], cpu_regs[src]);

                    TCGLabel *l = gen_new_label();
                    tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_regs[src], 0, l);

                    tcg_gen_movi_tl(cpu_reg_pc, ctx->pc-(dst<<1));
                    tcg_gen_exit_tb(NULL, 0);

                    gen_set_label(l);

                    tcg_gen_movi_tl(cpu_reg_pc, ctx->pc);
                    tcg_gen_exit_tb(NULL, 0);

                    ctx->bstate = BS_BRANCH;
                }
                break;
            default:
                return 0;
        }
    }
    return 1;
}

static inline int decode_sop(CPUK1801VM1State *env, DisasContext *ctx, int op)
{
    int oppart, b, addr, store;

    addr = op & 0000077;
    b      = op & 0100000;
    oppart = op & 0077700;

    if (oppart != 000300 && (oppart < 005000 || oppart > 006700))
        return 0;

    store = (oppart == 006400) ? 1 : 0;


    TCGv t = tcg_temp_new_i32();
    load_operand(env, ctx, t, addr, b, store);

    store = !store;

    switch (oppart) {
        case 000300:            // 0003 	SWAB 	Swap bytes: rotate 8 bits
            tcg_gen_bswap16_tl(t, t);
            break;
        case 005000:            // 0050 	CLR(B) 	Clear: dest = 0
            tcg_gen_movi_tl(t, 0);
            tcg_gen_movi_tl(cpu_reg_psw, 0b0100);
            break;
        case 005100:            // 0051 	COM(B) 	Complement: dest = ~dest
            tcg_gen_not_tl(t, t);
            cc_save_zn(t, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
            cc_set_cv(1, 0);
            break;
        case 005200:            // 0052 	INC(B) 	Increment: dest += 1
            {
                TCGv t1 = tcg_temp_new_i32();
                TCGv t2 = tcg_temp_new_i32();
                tcg_gen_mov_tl(t1, t);
                tcg_gen_movi_tl(t2, 1);
                tcg_gen_add_tl(t, t1, t2);
                cc_save_zn(t, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                cc_save_cv(t1, t2, t,
                            CC_PSW_MASK_C_CH | CC_PSW_MASK_V_CH,
                            CC_CMD_ADD);
                tcg_temp_free_i32(t1);
                tcg_temp_free_i32(t2);
            }
            break;
        case 005300:            // 0053 	DEC(B) 	Decrement: dest −= 1
            {
                TCGv t1 = tcg_temp_new_i32();
                TCGv t2 = tcg_temp_new_i32();
                tcg_gen_mov_tl(t1, t);
                tcg_gen_movi_tl(t2, 1);
                tcg_gen_sub_tl(t, t1, t2);
                cc_save_zn(t, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                cc_save_cv(t1, t2, t,
                            CC_PSW_MASK_C_CH | CC_PSW_MASK_V_CH,
                            CC_CMD_SUB);
                tcg_temp_free_i32(t1);
                tcg_temp_free_i32(t2);
            }
            break;
        case 005400:            // 0054 	NEG(B) 	Negate: dest = −dest
            tcg_gen_neg_tl(t, t);
            break;
        case 005500:            // 0055 	ADC(B) 	Add carry: dest += C
            {
                TCGv t1 = tcg_temp_new_i32();
                TCGv t2 = tcg_temp_new_i32();

                tcg_gen_mov_tl(t1, t);
//                 cc_gen_psw(ctx);
//                 tcg_gen_andi_tl(t2, cpu_reg_psw, CC_PSW_MASK_C); // TODO it's a hack!!!
                tcg_gen_movi_tl(t2, 0);
                tcg_gen_add_tl(t, t1, t2);
                cc_save_zn(t, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                cc_save_cv(t1, t2, t,
                            CC_PSW_MASK_C_CH | CC_PSW_MASK_V_CH,
                            CC_CMD_SUB);

                tcg_temp_free_i32(t1);
                tcg_temp_free_i32(t2);
            }
            break;
        case 005600:            // 0056 	SBC(B) 	Subtract carry: dest −= C
            tcg_gen_subi_tl(t, t, 1); // TODO sub carry not 1
            printf("dumb for SBC\n");
            break;
        case 005700:            // 0057 	TST(B) 	Test: Load src, set flags only
            cc_save_zn(t, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
            cc_set_cv(0, 0);
            break;
        case 006000:            // 0060 	ROR(B) 	Rotate right 1 bit
            tcg_gen_rotri_tl(t, t, 1); // TODO check
            break;
        case 006100:            // 0061 	ROL(B) 	Rotate left 1 bit
            tcg_gen_rotli_tl(t, t, 1); // TODO check
            break;
        case 006200:            // 0062 	ASR(B) 	Shift right: dest >>= 1
            tcg_gen_shri_tl(t, t, 1); // TODO check
            break;
        case 006300:            // 0063 	ASL(B) 	Shift left: dest <<= 1
            {
                TCGv t1 = tcg_temp_new_i32();
                tcg_gen_mov_tl(t1, t);
                tcg_gen_shli_tl(t, t, 1); // TODO check
                cc_save_zn(t, CC_PSW_MASK_Z_CH | CC_PSW_MASK_N_CH);
                cc_save_cv(t1, t1, t, CC_PSW_MASK_C_CH | CC_PSW_MASK_V_CH, CC_CMD_ASL);
                tcg_temp_free_i32(t1);
            }
            break;
        case 006400:
            if (b) {            // 1064 	MTPS 	Move to status: PS = src
                tcg_gen_mov_tl(cpu_reg_psw, t);
            } else {            // 0064 	MARK 	Return from subroutine, skip 0..63 instruction words
                printf("dumb for MARK\n"); // TODO
            }
            break;
        case 006500:            // 0065 	MFPI 	Move from previous I space: −(SP) = src
            if (b) {
                printf("dumb for MFPI\n"); // TODO
            } else {            // 1065 	MFPD 	Move from previous D space: −(SP) = src
                printf("dumb for MFPD\n"); // TODO
            }
            break;
        case 006600:
            if (b) {            // 0066 	MTPI 	Move to previous I space: dest = (SP)+
                printf("dumb for MTPI\n"); // TODO
            } else {            // 1066 	MTPD 	Move to previous D space: dest = (SP)+
                printf("dumb for MTPD\n"); // TODO
            }
            break;
        case 006700:
            if (b) {            // 0067 	SXT 	Sign extend: dest = (16 copies of N flag)
                printf("dumb for SXT\n"); // TODO
            } else {            // 1067 	MFPS 	Move from status: dest = PS
                printf("dumb for MFPS\n"); // TODO
            }
            break;
        default:
            return 0;
    }

    if (store)
        store_operand(env, ctx, t, addr, b, 1);
    tcg_temp_free_i32(t);

    return 1;
}

// cond is reversed
#define BRANCH(ctx, addr, mask, cond)               \
{                                                   \
    cc_gen_psw(ctx);                                \
    tcg_gen_movi_tl(cpu_reg_pc, ctx->pc);           \
    TCGv t = tcg_temp_new_i32();                    \
    TCGLabel *l = gen_new_label();                  \
    tcg_gen_andi_tl(t, cpu_reg_psw, mask);          \
    tcg_gen_brcondi_i32(cond, t, 0, l);             \
    tcg_gen_movi_tl(cpu_reg_pc, ctx->pc + addr);    \
    gen_set_label(l);                               \
    tcg_temp_free_i32(t);                           \
}

static inline int decode_branch(CPUK1801VM1State *env, DisasContext *ctx, int op)
{
    int oppart, reg, addr;

    if ((op & 0777000) == 0004000) {                // JSR 	Jump to subroutine
        reg = (op & 0000700) >> 6;

        load_addr(env, ctx, op & 0000077);

        if (reg != 7) {
            push(ctx, cpu_regs[reg]);
            tcg_gen_movi_tl(cpu_regs[reg], ctx->pc);
        } else {
            TCGv t = tcg_temp_new_i32();
            tcg_gen_movi_tl(t, ctx->pc);
            push(ctx, t);
            tcg_temp_free_i32(t);
        }
    } else if ((op & 0777000) == 0104000) { // EMT 	Emulator trap
        TCGv t = tcg_temp_new_i32();

        tcg_gen_movi_tl(cpu_reg_pc, ctx->pc);
        push(ctx, cpu_reg_psw);
        push(ctx, cpu_reg_pc);

        tcg_gen_movi_i32(t, 030);   // EMT vector
        tcg_gen_qemu_ld16u(cpu_reg_pc, t, ctx->memidx);

        tcg_gen_movi_i32(t, 032);   // EMT PSW
        tcg_gen_qemu_ld16u(cpu_reg_psw, t, ctx->memidx);

        tcg_temp_free_i32(t);

    } else if (op == 0000002) { // RTI 	Return from interrupt
        pop(ctx, cpu_reg_pc);
        pop(ctx, cpu_reg_psw);
    } else if ((op & 0777700) == 0100) { //      00001DD	JMP 	Jump
        load_addr(env, ctx, op & 0000077);
    } else if ((op & 0777770) == 0200) { //      000020R RTS 	Return from subroutine
        reg = op & 0000007;
        if (reg != 7) {
            tcg_gen_mov_tl(cpu_reg_pc, cpu_regs[reg]);
            pop(ctx, cpu_regs[reg]);
        } else
            pop(ctx, cpu_reg_pc);
    } else {
        oppart = op & 0777400;
        addr = (char)(op & 0000377);
        addr <<= 1;
        switch (oppart) {
            case 0000400:    //     0004xx 	BR  	Branch always
                tcg_gen_movi_tl(cpu_reg_pc, ctx->pc + addr);
                break;
            case 0001000:    //     0010xx 	BNE 	Branch if not equal (Z=0)
                BRANCH(ctx, addr, CC_PSW_MASK_Z, TCG_COND_NE);
                break;
            case 0001400: //     0014xx 	BEQ 	Branch if equal (Z=1)
                BRANCH(ctx, addr, CC_PSW_MASK_Z, TCG_COND_EQ);
                break;
    //         case 0002000: //     0020xx 	BGE 	Branch if greater than or equal (N^V = 0)
    //             break;
    //         case 0002400: //     0024xx 	BLT 	Branch if less than (N^V = 1)
    //             break;
    //         case 0003000: //     0030xx 	BGT 	Branch if greater than (Z|(N^V) = 0)
    //             break;
            case 0100000: //     1000xx 	BPL 	Branch if plus (N=0)
                BRANCH(ctx, addr, CC_PSW_MASK_N, TCG_COND_NE);
                break;
            case 0100400: //     1004xx 	BMI 	Branch if minus (N=1)
                BRANCH(ctx, addr, CC_PSW_MASK_N, TCG_COND_EQ);
                break;
            case 0101000: //     1010xx 	BHI 	Branch if higher than (C|Z = 0)
                BRANCH(ctx, addr, CC_PSW_MASK_C | CC_PSW_MASK_Z, TCG_COND_NE);
                break;
    //         case 0101400: //     1014xx 	BLOS 	Branch if lower or same (C|Z = 1)
    //             break;
            case 0102000: //     1020xx 	BVC 	Branch if overflow clear (V=0)
                BRANCH(ctx, addr, CC_PSW_MASK_V, TCG_COND_NE);
                break;
            case 0102400: //     1024xx 	BVS 	Branch if overflow set (V=1)
                BRANCH(ctx, addr, CC_PSW_MASK_V, TCG_COND_EQ);
                break;
            case 0103000: //     1030xx 	BCC  	Branch if carry clear (C=0)
                BRANCH(ctx, addr, CC_PSW_MASK_C, TCG_COND_NE);
                break;
            case 0103400: //     1034xx 	BCS 	Branch if carry set (C=1)
                BRANCH(ctx, addr, CC_PSW_MASK_C, TCG_COND_EQ);
                break;
            default:
                return 0;
        }
    }

    tcg_gen_exit_tb(NULL, 0);
    ctx->bstate = BS_BRANCH;

    return 1;
}

static inline int decode_nop(CPUK1801VM1State *env, DisasContext *ctx, int op)
{
    printf("nop %o\n", op);
    if (op == 0000240 || op == 0000260) {
        return 1;
    } else if ((op & 0777760) == 0000240) {
        tcg_gen_andi_tl(cpu_reg_psw, cpu_reg_psw, ~(op & 017));
    } else if ((op & 0777760) == 0000260) {
        tcg_gen_ori_tl(cpu_reg_psw, cpu_reg_psw, (op & 017));
    } else {
        switch (op) {
            default:
                return 0;
        }
    }
    return 1;
}

static void decode_opc(K1801VM1CPU *cpu, DisasContext *ctx)
{
    CPUK1801VM1State *env = &cpu->env;
    int op;

    op = ctx->opcode;

    ctx->pc += 2;

    if (decode_dop(env, ctx, op))
        return;

    if (decode_sop(env, ctx, op))
        return;

    if (decode_branch(env, ctx, op))
        return;

    if (decode_nop(env, ctx, op))
        return;

    printf("Unknown opcode: %06o (0x%02x)\n", ctx->opcode, ctx->opcode);
    exit(-1);
}

void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    CPUK1801VM1State *env = cs->env_ptr;
    K1801VM1CPU *cpu = env_archcpu(env);
    DisasContext ctx;
    target_ulong pc_start;
    int num_insns;

    pc_start = tb->pc;
    ctx.pc = pc_start;
    ctx.tb = tb;
    ctx.memidx = 0;
    ctx.bstate = BS_NONE;
    num_insns = 0;

    gen_tb_start(tb);

    do {
        tcg_gen_insn_start(ctx.pc);
        num_insns++;

        ctx.opcode = cpu_lduw_code(env, ctx.pc);
        print_regs(env, &ctx);
        decode_opc(cpu, &ctx);

        if (num_insns >= max_insns) {
            break;
        }
        if (cs->singlestep_enabled) {
            break;
        }
//         if ((ctx.pc & (TARGET_PAGE_SIZE - 1)) == 0) { TODO it is something useful, but I had a fault on it
//             break;
//         }

// SINGLE STEP TEST
//         if (ctx.pc == 0x84be)      // TEST
//             sleep(1);
//         else
            tcg_gen_movi_tl(cpu_reg_pc, ctx.pc); // TEST
        cc_gen_psw(&ctx); // TEST

        tcg_gen_exit_tb(NULL, 0); // TEST
        ctx.bstate = BS_BRANCH;   // TEST

    } while (ctx.bstate == BS_NONE && !tcg_op_buf_full());

    if (cs->singlestep_enabled) {
        tcg_gen_movi_tl(cpu_reg_pc, ctx.pc);
//         gen_helper_debug(cpu_env); TODO
    } else {
        switch (ctx.bstate) {
        case BS_STOP:
        case BS_NONE:
//             gen_goto_tb(env, &ctx, 0, ctx.pc); TODO
            break;
        case BS_EXCP:
//             tcg_gen_exit_tb(NULL, 0); TODO
            break;
        case BS_BRANCH:
        default:
            break;
        }
    }
    gen_tb_end(tb, num_insns);

    tb->size = ctx.pc - pc_start;
    tb->icount = num_insns;

}

void restore_state_to_opc(CPUK1801VM1State *env, TranslationBlock *tb,
                          target_ulong *data)
{
    env->regs[7] = data[0];
}

