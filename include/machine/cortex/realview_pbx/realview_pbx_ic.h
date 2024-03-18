// EPOS Realview PBX (ARM Cortex-A9) IC Mediator Declarations

#ifndef __realview_pbx_ic_h
#define __realview_pbx_ic_h

#include <machine/ic.h>
#include <machine/cortex/engine/cortex_a9/gic.h>
#include <system/memory_map.h>

__BEGIN_SYS

class IC_Engine: public IC_Common
{
public:
    // Interrupts
    static const unsigned int EXCS = CPU::EXCEPTIONS;
    static const unsigned int IRQS = GIC::IRQS;
    static const unsigned int INTS = EXCS + IRQS;

    enum {
        INT_SYS_TIMER   = EXCS + GIC::IRQ_PRIVATE_TIMER,
        INT_TIMER0      = EXCS + GIC::IRQ_GLOBAL_TIMER,
        INT_TIMER1      = UNSUPPORTED_INTERRUPT,
        INT_TIMER2      = UNSUPPORTED_INTERRUPT,
        INT_TIMER3      = UNSUPPORTED_INTERRUPT,
        INT_TSC_TIMER   = INT_TIMER0, // TSC must be disabled to use User_Timer
        INT_USR_TIMER   = INT_TIMER0,
        INT_GPIOA       = EXCS + GIC::IRQ_GPIO,
        INT_GPIOB       = EXCS + GIC::IRQ_GPIO,
        INT_GPIOC       = EXCS + GIC::IRQ_GPIO,
        INT_GPIOD       = EXCS + GIC::IRQ_GPIO,
        INT_NIC0_RX     = EXCS + GIC::IRQ_ETHERNET0,
        INT_NIC0_TX     = EXCS + GIC::IRQ_ETHERNET0,
        INT_NIC0_ERR    = EXCS + GIC::IRQ_ETHERNET0,
        INT_USB0        = EXCS + GIC::IRQ_USB0
    };

public:
    static void enable() { gic_distributor()->enable(); }
    static void enable(Interrupt_Id i)  { if((i >= EXCS) && (i <= INTS)) gic_distributor()->enable(int2irq(i)); }
    static void disable() { gic_distributor()->disable(); }
    static void disable(Interrupt_Id i) { if((i >= EXCS) && (i <= INTS)) gic_distributor()->disable(int2irq(i)); }

    static Interrupt_Id int_id() { return irq2int(gic_cpu()->int_id()); }
    static Interrupt_Id irq2int(Interrupt_Id i) { return i + EXCS; }
    static Interrupt_Id int2irq(Interrupt_Id i) { return i - EXCS; }

    static void ipi(unsigned int cpu, Interrupt_Id i) { gic_distributor()->send_sgi(cpu, int2irq(i)); }

    static void init() {
        gic_distributor()->init();
        gic_cpu()->init();
    };

private:
    static GIC_CPU * gic_cpu() { return reinterpret_cast<GIC_CPU *>(Memory_Map::GIC_CPU_BASE); }
    static GIC_Distributor * gic_distributor() { return reinterpret_cast<GIC_Distributor *>(Memory_Map::GIC_DIST_BASE); }
};

__END_SYS

#endif
