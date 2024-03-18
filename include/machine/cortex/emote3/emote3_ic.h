// EPOS EPOSMote III (ARM Cortex-M3) IC Mediator Declarations

#ifndef __emote3_ic_h
#define __emote3_ic_h

#include <machine/ic.h>
#include <machine/cortex/engine/cortex_m3/nvic.h>
#include <system/memory_map.h>

__BEGIN_SYS

class IC_Engine: public IC_Common
{
public:
    // Interrupts
    static const unsigned int EXCS = CPU::EXCEPTIONS;
    static const unsigned int IRQS = NVIC::IRQS;
    static const unsigned int INTS = EXCS + IRQS;

    enum : unsigned int {
        INT_HARD_FAULT  = CPU::EXC_HARD,
        INT_SYS_TIMER   = CPU::EXC_SYSTICK,

        INT_TIMER0      = EXCS + NVIC::IRQ_GPT0A,
        INT_TIMER1      = EXCS + NVIC::IRQ_GPT1A,
        INT_TIMER2      = EXCS + NVIC::IRQ_GPT2A,
        INT_TIMER3      = EXCS + NVIC::IRQ_GPT3A,
        INT_USR_TIMER   = INT_TIMER0,
        INT_TSC_TIMER   = INT_TIMER3,
        INT_GPIOA       = EXCS + NVIC::IRQ_GPIOA,
        INT_GPIOB       = EXCS + NVIC::IRQ_GPIOB,
        INT_GPIOC       = EXCS + NVIC::IRQ_GPIOC,
        INT_GPIOD       = EXCS + NVIC::IRQ_GPIOD,
        INT_NIC0_RX     = EXCS + NVIC::IRQ_RFTXRX,
        INT_NIC0_TX     = EXCS + NVIC::IRQ_RFTXRX,
        INT_NIC0_ERR    = EXCS + NVIC::IRQ_RFERR,
        INT_NIC0_TIMER  = EXCS + NVIC::IRQ_MACTIMER,
        INT_USB0        = EXCS + NVIC::IRQ_USB
    };

public:
    static void enable() { nvic()->enable(); }
    static void enable(Interrupt_Id i)  { if((i >= EXCS) && (i <= INTS)) nvic()->enable(int2irq(i)); }
    static void disable() { nvic()->disable(); }
    static void disable(Interrupt_Id i) { if((i >= EXCS) && (i <= INTS)) nvic()->disable(int2irq(i)); }

    static Interrupt_Id int_id() { return irq2int(nvic()->int_id()); }  // only works in handler mode (inside IC::entry())
    static Interrupt_Id irq2int(Interrupt_Id i) { return i + EXCS; }
    static Interrupt_Id int2irq(Interrupt_Id i) { return i - EXCS; }

    static void ipi(unsigned int cpu, Interrupt_Id i) {} // Cortex-M3 is always single-core

    static void init() { nvic()->init(); };

private:
    static NVIC * nvic() { return reinterpret_cast<NVIC *>(Memory_Map::SCB_BASE); }
};

__END_SYS

#endif
