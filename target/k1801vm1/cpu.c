#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"
#include "machine.h"
#include "exec/cpu_ldst.h"


static void k1801vm1_cpu_set_pc(CPUState *cs, vaddr value)
{
    K1801VM1CPU *cpu = K1801VM1_CPU(cs);
    cpu->env.regs[7] = value;
}

static bool k1801vm1_cpu_has_work(CPUState *cs)
{
    return cs->interrupt_request & CPU_INTERRUPT_HARD;
}

static void k1801vm1_cpu_reset(CPUState *s)
{
    K1801VM1CPU *cpu = K1801VM1_CPU(s);
    K1801VM1CPUClass *mcc = K1801VM1_CPU_GET_CLASS(cpu);
    CPUK1801VM1State *env = &cpu->env;
    int i;

    mcc->parent_reset(s);

    memset(env, 0, offsetof(CPUK1801VM1State, end_reset_fields));

    for (i = 0; i < 7; i++)
        env->regs[i] = 0;
    env->regs[7] = 0100000;
    env->psw.word = 0;
}

static void k1801vm1_cpu_disas_set_info(CPUState *cpu, disassemble_info *info)
{
    info->mach = bfd_arch_k1801vm1;
    info->print_insn = NULL;
}

static void k1801vm1_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    K1801VM1CPUClass *mcc = K1801VM1_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

    cpu_exec_realizefn(cs, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }

    qemu_init_vcpu(cs);
    cpu_reset(cs);

    mcc->parent_realize(dev, errp);
}

static void k1801vm1_cpu_initfn(Object *obj)
{
    K1801VM1CPU *cpu = K1801VM1_CPU(obj);
    cpu_set_cpustate_pointers(cpu);
}

static bool k1801vm1_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    K1801VM1CPU *cpu = K1801VM1_CPU(cs);
    CPUK1801VM1State *env = &cpu->env;

    if (interrupt_request & CPU_INTERRUPT_HARD) {
        if (interrupt_request & CPU_INTERRUPT_TGT_EXT_0) { // TODO replace to normal define
            env->regs[6] -= 2;
            cpu_stw_data(env, env->regs[6], env->psw.word);

            env->regs[6] -= 2;
            cpu_stw_data(env, env->regs[6], env->regs[7]);

            env->regs[7] = cpu_lduw_code(env, 060);
            env->psw.word = cpu_lduw_code(env, 062);
        }
        cs->interrupt_request = 0;
        return true;
    }
    return false;
}

static ObjectClass *k1801vm1_cpu_class_by_name(const char *cpu_model)
{
    ObjectClass *oc;
    char *typename;

    typename = g_strdup_printf(K1801VM1_CPU_TYPE_NAME("%s"), cpu_model);
    oc = object_class_by_name(typename);
    g_free(typename);
    if (oc != NULL && (!object_class_dynamic_cast(oc, TYPE_K1801VM1_CPU) ||
                       object_class_is_abstract(oc))) {
        return NULL;
    }
    return oc;
}

static void k1801vm1_cpu_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    K1801VM1CPUClass *mcc = K1801VM1_CPU_CLASS(oc);

    device_class_set_parent_realize(dc, k1801vm1_cpu_realizefn, &mcc->parent_realize);
    mcc->parent_reset = cc->reset;
    cc->reset = k1801vm1_cpu_reset;

    cc->class_by_name = k1801vm1_cpu_class_by_name;

    cc->has_work = k1801vm1_cpu_has_work;
    cc->do_interrupt = k1801vm1_cpu_do_interrupt;
    cc->cpu_exec_interrupt = k1801vm1_cpu_exec_interrupt;
    cc->dump_state = k1801vm1_cpu_dump_state;
    cc->set_pc = k1801vm1_cpu_set_pc;
    cc->tlb_fill = k1801vm1_cpu_tlb_fill;
    cc->get_phys_page_debug = k1801vm1_cpu_get_phys_page_debug;
    cc->vmsd = &vmstate_k1801vm1_cpu;
    cc->disas_set_info = k1801vm1_cpu_disas_set_info;
    cc->tcg_initialize = k1801vm1_translate_init;
}


static const TypeInfo k1801vm1_cpus_type_infos[] = {
    {
        .name = TYPE_K1801VM1_CPU,
        .parent = TYPE_CPU,
        .instance_size = sizeof(K1801VM1CPU),
        .instance_init = k1801vm1_cpu_initfn,
        .class_size = sizeof(K1801VM1CPUClass),
        .class_init = k1801vm1_cpu_class_init,
    },
};

DEFINE_TYPES(k1801vm1_cpus_type_infos)