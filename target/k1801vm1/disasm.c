#include "disasm.h"
#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "tcg-op.h"
#include <stdio.h>


static CPUK1801VM1State *env = NULL;
static int pc;


static const char *regname(int reg)
{
    static char res[4];
    if (reg == 6)
        return "sp";
    if (reg == 7)
        return "pc";
    snprintf(res, sizeof(res), "r%d", reg & 7);
    return res;
}

static const char *operand(int addr)
{
    int mode, reg;
    static char res[32];

    mode = (addr & 070) >> 3;
    reg = addr & 007;

    snprintf(res, sizeof(res), "%d[%d]", mode, reg);

    switch (mode) {
        case 0:
            return regname(reg);
        case 1:
            if (reg == 7)
                snprintf(res, sizeof(res), "@%s", regname(reg));
            else
                snprintf(res, sizeof(res), "(%s)", regname(reg));
            break;
        case 2:
            if (reg == 7) {
                snprintf(res, sizeof(res), "#%o", cpu_lduw_code(env, pc));
                pc += 2;
            }
            else
                snprintf(res, sizeof(res), "(%s)+", regname(reg));
            break;
        case 3:
            if (reg == 7) {
                snprintf(res, sizeof(res), "@#%o", cpu_lduw_code(env, pc));
                pc += 2;
            }
            break;
        case 4:
            snprintf(res, sizeof(res), "-(%s)", regname(reg));
            break;
        case 5:
            snprintf(res, sizeof(res), "-@(%s)", regname(reg));
            break;
        case 6:
            if (reg == 7) {
                addr = (pc + 2 + cpu_lduw_code(env, pc)) & 0xffff;
                snprintf(res, sizeof(res), "%o", addr);
            } else
                snprintf(res, sizeof(res), "%o(%s)", cpu_lduw_code(env, pc), regname(reg));
            pc += 2;
            break;
        case 7:
            if (reg == 7) {
                addr = (pc + 2 + cpu_lduw_code(env, pc)) & 0xffff;
                snprintf(res, sizeof(res), "@%o", addr);
            } else
                snprintf(res, sizeof(res), "@%o(%s)", cpu_lduw_code(env, pc), regname(reg));
            pc += 2;
            break;
    }

    return res;
}

static int dop(int opcode, char *buf, int size)
{
    int oppart, src, dst, byte;
    char srcbuf[32];
    const char *op = NULL;

    oppart = opcode & 0070000;
    if (oppart == 0)
        return 0;

    byte = opcode & 0100000;
    dst = opcode & 0000077;

    if (oppart != 0070000) {
        src = (opcode & 0007700) >> 6;

        switch (oppart) {
            case 0010000:
                op = (byte) ? "movb" : "mov";
                break;
            case 0020000:
                op = (byte) ? "cmpb" : "cmp";
                break;
            case 0030000:
                op = (byte) ? "bitb" : "bit";
                break;
            case 0040000:
                op = (byte) ? "bicb" : "bic";
                break;
            case 0050000:
                op = (byte) ? "bisb" : "bis";
                break;
            case 0060000:
                op = (byte) ? "sub" : "add";
                break;
        }
    } else {
        oppart = opcode & 0007000;
        src = (opcode & 0000700) >> 6;
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
                op = "xor";
                break;
//             case 0005000: // floating point ops
//                 break;
//             case 0006000: // system instructions
//                 break;
            case 0007000:
                return snprintf(buf, size, "sob %s, %o", regname(src), pc - (dst * 2));
        }
    }
    if (!op)
        return 0;

    snprintf(srcbuf, sizeof(srcbuf), "%s", operand(src));

    return snprintf(buf, size, "%s %s, %s", op, srcbuf, operand(dst));
}

static int sop(int opcode, char *buf, int size)
{
    int addr, byte, oppart;
    const char *op = NULL;

    addr = opcode & 0000077;
    byte = opcode & 0100000;
    oppart = opcode & 0077700;

    if (oppart != 000300 && (oppart < 005000 || oppart > 006700))
        return 0;

    switch (oppart) {
        case 000300:            // 0003 	SWAB 	Swap bytes: rotate 8 bits
            op = "swab";
            break;
        case 005000:            // 0050 	CLR(B) 	Clear: dest = 0
            op = (byte) ? "clrb" : "clr";
            break;
        case 005100:            // 0051 	COM(B) 	Complement: dest = ~dest
            op = (byte) ? "comb" : "com";
            break;
        case 005200:            // 0052 	INC(B) 	Increment: dest += 1
            op = (byte) ? "incb" : "inc";
            break;
        case 005300:            // 0053 	DEC(B) 	Decrement: dest −= 1
            op = (byte) ? "decb" : "dec";
            break;
        case 005400:            // 0054 	NEG(B) 	Negate: dest = −dest
            op = (byte) ? "negb" : "neg";
            break;
        case 005500:            // 0055 	ADC(B) 	Add carry: dest += C
            op = (byte) ? "adcb" : "adc";
            break;
        case 005600:            // 0056 	SBC(B) 	Subtract carry: dest −= C
            op = (byte) ? "sbcb" : "sbc";
            break;
        case 005700:            // 0057 	TST(B) 	Test: Load src, set flags only
            op = (byte) ? "tstb" : "tst";
            break;
        case 006000:            // 0060 	ROR(B) 	Rotate right 1 bit
            op = (byte) ? "rorb" : "ror";
            break;
        case 006100:            // 0061 	ROL(B) 	Rotate left 1 bit
            op = (byte) ? "rolb" : "rol";
            break;
        case 006200:
            op = (byte) ? "asrb" : "asr";
            break;
        case 006300:
            op = (byte) ? "aslb" : "asl";
            break;
        case 006400:
            op = (byte) ? "mtps" : "mark";
            break;
        case 006500:
            op = (byte) ? "mfpi" : "mfpd";
            break;
        case 006600:
            op = (byte) ? "mtpi" : "mtpd";
            break;
        case 006700:
            op = (byte) ? "sxt" : "mfps";
            break;
    }
    if (!op)
        return 0;

//     pc = cpu_lduw_code(env, pc) + 2;
    return snprintf(buf, size, "%s %s", op, operand(addr));
}

static const char *branch_addr(int addr)
{
    int mode, reg;
    static char res[16];

    mode = addr & 070;
    reg =  addr & 007;

    if (reg == 7) {
        switch (mode) {
            case 030:
                addr = cpu_lduw_code(env, pc);
                break;
            case 060:
                addr = (pc + cpu_lduw_code(env, pc) + 2) & 0xffff;
                break;
        }
    } else {
        switch (mode) {
            case 010:
                snprintf(res, sizeof(res), "(%s)", regname(reg));
                return res;
            case 030:
//                 addr = cpu_lduw_code(env, env->regs[reg] & 0xffff);
                snprintf(res, sizeof(res), "@(%s)+", regname(reg));
                return res;
            case 070:
                addr = cpu_lduw_code(env, pc) & 0xffff;
                snprintf(res, sizeof(res), "@%o(r%d)", addr, reg);
                return res;
                break;
            default:
                printf("DISASM JUMP INCORRECT MODE %o (reg=%o)\n", mode, reg);
        }
    }
    snprintf(res, sizeof(res), "%o", addr);
    return res;
}

static int branch(int opcode, char *buf, int size)
{
    int oppart, addr;
    const char *op;

    if ((opcode & 0777000) == 0004000)
        return snprintf(buf, size, "jsr %s, %s", regname((opcode & 0700) >> 6), branch_addr(opcode & 077));
    if ((opcode & 0777000) == 0104000) {
        op = (opcode & 0000400) ? "trap" : "emt";
        return snprintf(buf, size, "%s %o", op, opcode & 0377);
    }
    if (opcode == 0000002)
        return snprintf(buf, size, "rti ");
    if ((opcode & 0777700) == 0100)
        return snprintf(buf, size, "jmp %s", branch_addr(opcode & 077));
    if ((opcode & 0777770) == 0200)
        return snprintf(buf, size, "rts %s ", regname(opcode & 07));

    oppart = opcode & 0777400;
    addr = opcode & 0377;
    if (addr & 0200)
        addr = addr - 0x100;
    addr <<= 1;

    switch (oppart) {
        case 0000400:
            op = "br";
            break;
        case 0001000:
            op = "bne";
            break;
        case 0001400:
            op = "beq";
            break;
        case 0002000:
            op = "bge";
            break;
        case 0002400:
            op = "blt";
            break;
        case 0003000:
            op = "bgt";
            break;
        case 0003400:
            op = "ble";
            break;
        case 0100000:
            op = "bpl";
            break;
        case 0100400:
            op = "bmi";
            break;
        case 0101000:
            op = "bhi";
            break;
        case 0101400:
            op = "blos";
            break;
        case 0102000:
            op = "bvc";
            break;
        case 0102400:
            op = "bvs";
            break;
        case 0103000:
            op = "bcc";
            break;
        case 0103400:
            op = "bcs";
            break;
        default:
            return 0;
    }
    return snprintf(buf, size, "%s %o", op, pc + addr);
}

static int nop(int opcode, char *buf, int size)
{
    const char *op = NULL;
    switch (opcode) {
        case 0000005:
            op = "reset";
            break;
        case 0240:
        case 0260:
            op = "nop";
            break;
        case 0241:
            op = "clc";
            break;
        case 0242:
            op = "clv";
            break;
        case 0244:
            op = "clz";
            break;
        case 0257:
            op = "ccc";
            break;
        case 0261:
            op = "sec";
            break;
        case 0262:
            op = "sev";
            break;
        case 0264:
            op = "sez";
            break;
        case 0270:
            op = "sen";
            break;
        case 0277:
            op = "scc";
            break;
        default:
            return 0;
    }
    return snprintf(buf, size, "%s", op);
}

const char *k1801vm1_disasm(void *opaque, int opcode)
{
    env = (CPUK1801VM1State *)opaque;
    pc = env->regs[7] + 2;
    static char res[32];

    if (dop(opcode, res, sizeof(res)) > 0)
        goto end;
    if (sop(opcode, res, sizeof(res)) > 0)
        goto end;
    if (branch(opcode, res, sizeof(res)) > 0)
        goto end;
    if (nop(opcode, res, sizeof(res)) > 0)
        goto end;
    snprintf(res, sizeof(res), "???");
end:
    return res;
}
