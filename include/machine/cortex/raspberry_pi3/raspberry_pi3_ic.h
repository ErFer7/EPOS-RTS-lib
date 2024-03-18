// EPOS Paspberry Pi3 (ARM Cortex-A53) IC Mediator Declarations

#ifndef __raspberry_pi3_ic_h
#define __raspberry_pi3_ic_h

#include <machine/cortex/engine/cortex_a53/bcm_mailbox.h>
#include <system/memory_map.h>

__BEGIN_SYS

class IC_Engine: public BCM_IC_Common
{
public:
    // Interrupts
    static const unsigned int EXCS = CPU::EXCEPTIONS;
    static const unsigned int IRQS = BCM_IC_Common::IRQS;
    static const unsigned int INTS = EXCS + IRQS;

    enum {
        INT_PREFETCH_ABORT      = CPU::EXC_PREFETCH_ABORT,
        INT_DATA_ABORT          = CPU::EXC_DATA_ABORT,

        INT_TIMER0              = EXCS + SYSTEM_TIMER_MATCH0, // used by the GPU
        INT_TIMER1              = EXCS + SYSTEM_TIMER_MATCH1,
        INT_TIMER2              = EXCS + SYSTEM_TIMER_MATCH2, // used by the GPU
        INT_TIMER3              = EXCS + SYSTEM_TIMER_MATCH3,
        INT_TIMER4              = EXCS + ARM_TIMER_IRQ,
        INT_TIMER5              = EXCS + MAILBOX_TIMER_IRQ,
        INT_SYS_TIMER           = Traits<Machine>::emulated ? INT_TIMER5 : INT_TIMER1,
        INT_USR_TIMER           = INT_TIMER3,
        INT_TSC_TIMER           = INT_TIMER4,
        INT_GPIOA               = EXCS + GPIO_INT0,
        INT_GPIOB               = EXCS + GPIO_INT1,
        INT_GPIOC               = EXCS + GPIO_INT2,
        INT_GPIOD               = EXCS + GPIO_INT3,
        INT_UART0               = EXCS + UART_AUX_INT,
        INT_UART1               = EXCS + UART_INT0,
        INT_USB0                = EXCS + USB_CONTROLLER,
        INT_SPI                 = EXCS + SPI_INT,
        INT_GPU                 = EXCS + MAILBOX_GPU_IRQ,
        INT_PMU                 = EXCS + MAILBOX_PMU_IRQ,
        INT_AIX                 = EXCS + MAILBOX_AIX_IRQ
    };

public:
    static void enable() { /* irq()->enable(); */ mbox()->enable(); } // FIXME

    static void enable(Interrupt_Id i) {
        assert(i <= INTS);
        Interrupt_Id j = int2irq(i);
        if(j < MAILBOX_FIRST_IRQ)
            irq()->enable(j);
        else
            mbox()->enable(j);
    }

    static void disable() { /* irq()->disable(); mbox()->disable(); */ } // FIXME

    static void disable(unsigned int i) {
        assert(i <= INTS);
        Interrupt_Id j = int2irq(i);
        if(j < MAILBOX_FIRST_IRQ)
            irq()->disable(j);
        else
            mbox()->disable(j);
    }

    static Interrupt_Id int_id() {
#ifdef __armv7__
        Interrupt_Id id; // ARMv7 exceptions are handled by physical handlers
#else
        Interrupt_Id id = CPU::esr_el1() >> CPU::EC_OFFSET; // check ARMv8 exceptions
        if(id) {
            if((id & CPU::EXC_PREFETCH_ABORT) == id) return INT_PREFETCH_ABORT;
            if((id & CPU::EXC_DATA_ABORT) == id) return INT_DATA_ABORT;
        }
#endif
        id = mbox()->int_id(); // check mailbox
        if(id == INT_UNKNOWN)  // if it wasn't the mailbox that triggered the interrupt, then check IRQ
            id = irq()->int_id();
        return EXCS + id;
}

    static Interrupt_Id irq2int(Interrupt_Id i) { return i + EXCS; }
    static Interrupt_Id int2irq(Interrupt_Id i) { return i - EXCS; }

    static void ipi(unsigned int cpu, Interrupt_Id id) { mbox()->ipi(cpu, id); }

    static void mailbox_eoi(Interrupt_Id i) { mbox()->eoi(i); }

    static void init() { irq()->init(); mbox()->init(); };

private:
    static BCM_IRQ * irq() { return reinterpret_cast<BCM_IRQ *>(Memory_Map::IC_BASE); }
    static BCM_Mailbox * mbox() { return reinterpret_cast<BCM_Mailbox *>(Memory_Map::MBOX_CTRL_BASE); }
};

__END_SYS

#endif
