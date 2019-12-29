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
    target_ulong pc, saved_pc;
//     uint32_t fp_status;
//     target_ulong btarget;
//     int singlestep_enabled;

} DisasContext;

static TCGv cpu_regs[K1801VM1_REG_NUM];
static TCGv cpu_reg_psw;

#define cpu_reg_pc cpu_regs[7]
#define cpu_reg_sp cpu_regs[6]

#include "exec/gen-icount.h"

void k1801vm1_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    K1801VM1CPU *cpu = K1801VM1_CPU(cs);
    CPUK1801VM1State *env = &cpu->env;
    int i;
    for (i = 0; i < 8; i++) {
        qemu_fprintf(f, "%d=0x%04x  ", i, (uint16_t)env->regs[i]);
    }
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
}

static void print_regs(CPUK1801VM1State *env, DisasContext *ctx)
{
    int i;
    for (i = 0; i < 8; i++)
        printf("%d=0x%04x  ", i, env->regs[i]);
    printf("\t[");
    for (i = 0; i < 8; i++)
        printf("%d=%06o  ", i, env->regs[i]);
    printf("]\n");
}

#define loadb(env, ctx, t, addr)    load_operand(env, ctx, t, addr, 1)
#define loadw(env, ctx, t, addr)    load_operand(env, ctx, t, addr, 0)
static void load_operand(CPUK1801VM1State *env, DisasContext *ctx, TCGv t, int addr, int byte)
{
    int mode, reg;

    mode = addr & 070;
    reg =  addr & 007;

    if (reg == 7) {
        switch (mode) {
            case 020:
                tcg_gen_movi_tl(t, (byte) ? cpu_ldub_code(env, ctx->pc) : cpu_lduw_code(env, ctx->pc));
                break;
            case 030:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_movi_tl(t, 0);
                    tcg_gen_movi_tl(t1, cpu_lduw_code(env, ctx->pc));
                    if (byte)
                        tcg_gen_qemu_ld8u(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16u(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;
            case 060:
                tcg_gen_movi_tl(t, ctx->pc + ((byte) ? cpu_ldsb_code(env, ctx->pc) : cpu_ldsw_code(env, ctx->pc)) + 2);
                break;
            case 070:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_movi_tl(t1, ctx->pc + cpu_lduw_code(env, ctx->pc)); // TODO check if the addr is right
                    if (byte)
                        tcg_gen_qemu_ld8u(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16u(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;

            default:
                printf("INCORRECT LOAD mode=%o, reg=7\n", mode); //TEST
                exit(-1); //TEST
        }
        ctx->pc += 2;
    } else {
        switch (mode) {
            case 000:
                tcg_gen_mov_tl(t, cpu_regs[reg]);
                break;
            case 010:
                tcg_gen_movi_tl(t, 0);
                if (byte)
                    tcg_gen_qemu_ld8u(t, cpu_regs[reg], ctx->memidx);
                else
                    tcg_gen_qemu_ld16u(t, cpu_regs[reg], ctx->memidx);
                break;
            case 020:
                tcg_gen_movi_tl(t, 0);
                if (byte)
                    tcg_gen_qemu_ld8u(t, cpu_regs[reg], ctx->memidx);
                else
                    tcg_gen_qemu_ld16u(t, cpu_regs[reg], ctx->memidx);
                tcg_gen_addi_tl(cpu_regs[reg], cpu_regs[reg], (byte) ? 1 : 2);
                break;
            case 030:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_movi_tl(t1, 0);
                    tcg_gen_qemu_ld16u(t1, cpu_regs[reg], ctx->memidx);
                    tcg_gen_movi_tl(t, 0);
                    if (byte)
                        tcg_gen_qemu_ld8u(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16u(t, t1, ctx->memidx);
                    tcg_gen_addi_tl(cpu_regs[reg], cpu_regs[reg], (byte) ? 1 : 2);
                    tcg_temp_free_i32(t1);
                }
                break;
            case 040:
                tcg_gen_movi_tl(t, 0);
                tcg_gen_subi_tl(cpu_regs[reg], cpu_regs[reg], (byte) ? 1 : 2);
                if (byte)
                    tcg_gen_qemu_ld8u(t, cpu_regs[reg], ctx->memidx);
                else
                    tcg_gen_qemu_ld16u(t, cpu_regs[reg], ctx->memidx);
                break;
            case 060:
                {
                    TCGv t1 = tcg_temp_new_i32();
                    tcg_gen_movi_tl(t, 0);
                    tcg_gen_addi_tl(t1, cpu_regs[reg], cpu_lduw_code(env, ctx->pc));
                    if (byte)
                        tcg_gen_qemu_ld8u(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_ld16u(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
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

#define storeb(env, ctx, t, addr)    store_operand(env, ctx, t, addr, 1)
#define storew(env, ctx, t, addr)    store_operand(env, ctx, t, addr, 0)
static void store_operand(CPUK1801VM1State *env, DisasContext *ctx, TCGv t, int addr, int byte)
{
    int mode, reg;

    mode = addr & 070;
    reg =  addr & 007;

    if (reg == 7) {
        switch (mode) {
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
                    tcg_gen_movi_tl(t1, ctx->pc + cpu_ldsw_code(env, ctx->pc) + 2);
                    if (byte)
                        tcg_gen_qemu_st8(t, t1, ctx->memidx);
                    else
                        tcg_gen_qemu_st16(t, t1, ctx->memidx);
                    tcg_temp_free_i32(t1);
                }
                break;
            default:
                printf("INCORRECT STORE mode=%o, reg=7\n", mode); //TEST
                exit(-1); //TEST
        }
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
            case 020:
                if (byte)
                    tcg_gen_qemu_st8(t, cpu_regs[reg], ctx->memidx);
                else
                    tcg_gen_qemu_st16(t, cpu_regs[reg], ctx->memidx);
                tcg_gen_addi_tl(cpu_regs[reg], cpu_regs[reg], (byte) ? 1 : 2);
                break;
            case 040:
                tcg_gen_subi_tl(cpu_regs[reg], cpu_regs[reg], (byte) ? 1 : 2);
                if (byte)
                    tcg_gen_qemu_st8(t, cpu_regs[reg], ctx->memidx);
                else
                    tcg_gen_qemu_st16(t, cpu_regs[reg], ctx->memidx);
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
            case 060:
                {
                    ctx->pc += cpu_ldsw_code(env, ctx->pc) + 2;
                    tcg_gen_movi_tl(cpu_reg_pc, ctx->pc);
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
                break;
            default:
                printf("!!!!!!!!!!!!! JUMP INCORRECT MODE %o (reg=%o)\n", mode, reg);
                exit(-1); //TEST
        }
    }
}

static inline void push(DisasContext *ctx, TCGv t)
{
    tcg_gen_subi_tl(cpu_reg_sp, cpu_reg_sp, 2);
    tcg_gen_qemu_st16(t, cpu_reg_sp, ctx->memidx);
}

static inline void pop(DisasContext *ctx, TCGv t)
{
    tcg_gen_movi_tl(t, 0);
    tcg_gen_qemu_ld16s(t, cpu_reg_sp, ctx->memidx);
    tcg_gen_andi_tl(t, t, 0xffff);           //TODO ERROR ffff805e
    tcg_gen_addi_tl(cpu_reg_sp, cpu_reg_sp, 2);
}

static inline void set_psw(TCGv t)
{
    // TODO JUST DO IT
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
                {
                    TCGv t = tcg_temp_new_i32();
                    load_operand(env, ctx, t, src, b);
                    store_operand(env, ctx, t, dst, b);
                    tcg_temp_free_i32(t);
                }
                break;
//             case 0020000: // CMP
//                 break;
//             case 0030000: // BIT
//                 break;
            case 0040000: // BIC
                {
                    TCGv t1 = tcg_temp_new_i32();
                    TCGv t2 = tcg_temp_new_i32();
                    load_operand(env, ctx, t1, src, b);
                    load_operand(env, ctx, t2, dst, b);
                    tcg_gen_andc_tl(t2, t2, t1);       // dest &= ~src
                    store_operand(env, ctx, t2, dst, b);
                    tcg_temp_free_i32(t1);
                    tcg_temp_free_i32(t2);
                }
                break;
//             case 0050000: // BIS
//                 break;
//             case 0060000: // ADD/SUB
//                 break;
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
//             case 0004000: // XOR
//                 break;
//             case 0005000: // floating point ops
//                 break;
//             case 0006000: // system instructions
//                 break;
            case 0007000:                   // SOB    Subtract one and branch
                {
                    tcg_gen_subi_tl(cpu_regs[src], cpu_regs[src], 1);

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
    int oppart, b, addr;

    addr = op & 0000077;

    b      = op & 0100000;
    oppart = op & 0077700;

    if (oppart != 000300 && (oppart < 005000 || oppart > 006700))
        return 0;

    TCGv t = tcg_temp_new_i32();
    load_operand(env, ctx, t, addr, b);
    if ((addr & 07) == 7)
        ctx->pc -= 2; // we store the same operand as loaded

    switch (oppart) {
        case 000300:            // 0003 	SWAB 	Swap bytes: rotate 8 bits
            tcg_gen_bswap16_tl(t, t);
            break;
        case 005000:            // 0050 	CLR(B) 	Clear: dest = 0
            tcg_gen_movi_tl(t, 0);
            break;
        case 005100:            // 0051 	COM(B) 	Complement: dest = ~dest
            tcg_gen_not_tl(t, t);
            break;
        case 005200:            // 0052 	INC(B) 	Increment: dest += 1
            tcg_gen_addi_tl(t, t, 1);
            break;
        case 005300:            // 0053 	DEC(B) 	Decrement: dest −= 1
            tcg_gen_subi_tl(t, t, 1);
            break;
        case 005400:            // 0054 	NEG(B) 	Negate: dest = −dest
            tcg_gen_neg_tl(t, t);
            break;
        case 005500:            // 0055 	ADC(B) 	Add carry: dest += C
            tcg_gen_addi_tl(t, t, 1); // TODO add carry not 1
            printf("dumb for ADC\n");
            break;
        case 005600:            // 0056 	SBC(B) 	Subtract carry: dest −= C
            tcg_gen_subi_tl(t, t, 1); // TODO sub carry not 1
            printf("dumb for SBC\n");
            break;
        case 005700:            // 0057 	TST(B) 	Test: Load src, set flags only
            printf("dumb for TST\n"); // TODO
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
            tcg_gen_shli_tl(t, t, 1); // TODO check
            break;
        case 006400:
            if (b) {            // 0064 	MARK 	Return from subroutine, skip 0..63 instruction words
                printf("dumb for MARK\n"); // TODO
            } else {            // 1064 	MTPS 	Move to status: PS = src
                printf("dumb for MTPS\n"); // TODO
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

    store_operand(env, ctx, t, addr, b);
    tcg_temp_free_i32(t);

    return 1;
}

static inline int decode_branch(CPUK1801VM1State *env, DisasContext *ctx, int op)
{
    int oppart, reg, addr;

    if ((op & 0777000) == 0004000) {                // JSR 	Jump to subroutine
        reg = (op & 0000700) >> 6;
        if (reg != 7) {
            push(ctx, cpu_regs[reg]);
            tcg_gen_movi_tl(cpu_regs[reg], ctx->pc);
        } else {
            tcg_gen_movi_tl(cpu_reg_pc, ctx->pc);
            push(ctx, cpu_reg_pc);
        }

        load_addr(env, ctx, op & 0000077);
    } else if ((op & 0777000) == 0104000) {         // EMT 	Emulator trap
        TCGv t = tcg_temp_new_i32();

        tcg_gen_movi_tl(cpu_reg_pc, ctx->pc);
        push(ctx, cpu_reg_psw);
        push(ctx, cpu_reg_pc);

        tcg_gen_movi_i32(t, 030);                       // EMT vector
        tcg_gen_qemu_ld16u(cpu_reg_pc, t, ctx->memidx);

        tcg_gen_movi_i32(t, 032);                       // EMT PSW
        tcg_gen_qemu_ld16u(cpu_reg_psw, t, ctx->memidx);

        tcg_temp_free_i32(t);

    } else if (op == 0000002) {                     // RTI 	Return from interrupt
        pop(ctx, cpu_reg_pc);
        pop(ctx, cpu_reg_psw);
    } else {
        oppart = op & 0777700;
        addr   = op & 0000077;
        switch (oppart) {
            case 0000100:                           //      00001DD	JMP 	Jump
                load_addr(env, ctx, op & 0000077);
                break;
            case 0000200:                           //      000020R RTS 	Return from subroutine
                if (op & 0000070)
                    return 0;
                reg = op & 0000007;
                if (reg != 7) {
                    tcg_gen_mov_tl(cpu_reg_pc, cpu_regs[reg]);
                    pop(ctx, cpu_regs[reg]);
                } else
                    pop(ctx, cpu_reg_pc);
                break;
            case 0000400:                           //     0004xx 	BR  	Branch always
                addr <<= 1;
                tcg_gen_movi_tl(cpu_reg_pc, ctx->pc + addr);
                break;
    //         case 0001000: //     0010xx 	BNE 	Branch if not equal (Z=0)
    //             break;
    //         case 0001400: //     0014xx 	BEQ 	Branch if equal (Z=1)
    //             break;
    //         case 0002000: //     0020xx 	BGE 	Branch if greater than or equal (N^V = 0)
    //             break;
    //         case 0002400: //     0024xx 	BLT 	Branch if less than (N^V = 1)
    //             break;
    //         case 0003000: //     0030xx 	BGT 	Branch if greater than (Z|(N^V) = 0)
    //             break;
    //         case 0100000: //     1000xx 	BPL 	Branch if plus (N=0)
    //             break;
    //         case 0100400: //     1004xx 	BMI 	Branch if minus (N=1)
    //             break;
    //         case 0101000: //     1010xx 	BHI 	Branch if higher than (C|Z = 0)
    //             break;
    //         case 0101400: //     1014xx 	BLOS 	Branch if lower or same (C|Z = 1)
    //             break;
    //         case 0102000: //     1020xx 	BVC 	Branch if overflow clear (V=0)
    //             break;
    //         case 0102400: //     1024xx 	BVS 	Branch if overflow set (V=1)
    //             break;
    //         case 0103000: //     1030xx 	BCC or BHIS 	Branch if carry clear, or Branch if higher or same (C=0)
    //             break;
    //         case 0103400: //     1034xx 	BCS or BLO 	Branch if carry set, or Branch if lower than (C=1)
    //             break;
            default:
                return 0;
        }
    }

    tcg_gen_exit_tb(NULL, 0);
    ctx->bstate = BS_BRANCH;

    return 1;
}

static inline int decode_opsys(CPUK1801VM1State *env, DisasContext *ctx, int op)
{
    switch (op) {
        case 0000004:

            break;
        default:
            return 0;
    }
    return 1;
}

static void decode_opc(K1801VM1CPU *cpu, DisasContext *ctx)
{
    CPUK1801VM1State *env = &cpu->env;
    int op;

    op = ctx->opcode;

    printf("\tpc=0x%x opcode=%06o (0x%04x)\n", ctx->pc, op, op);

//     if (ctx->pc == 0x805e)
//         return;

    ctx->pc += 2;

    if (decode_dop(env, ctx, op))
        return;

    if (decode_sop(env, ctx, op))
        return;

    if (decode_branch(env, ctx, op))
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
    ctx.saved_pc = -1;
    ctx.tb = tb;
    ctx.memidx = 0;
    ctx.bstate = BS_NONE;
    num_insns = 0;

    print_regs(env, &ctx);

    gen_tb_start(tb);

    do {
        tcg_gen_insn_start(ctx.pc);
        num_insns++;

        ctx.opcode = cpu_lduw_code(env, ctx.pc);
        decode_opc(cpu, &ctx);

        if (num_insns >= max_insns) {
            break;
        }
        if (cs->singlestep_enabled) {
            break;
        }
        if ((ctx.pc & (TARGET_PAGE_SIZE - 1)) == 0) {
            break;
        }

        tcg_gen_movi_tl(cpu_reg_pc, ctx.pc); // TEST
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

