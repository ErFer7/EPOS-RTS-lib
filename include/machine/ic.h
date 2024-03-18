// EPOS IC Mediator Common Package

#ifndef __ic_h
#define __ic_h

#include <system/config.h>

__BEGIN_SYS

class IC_Common
{
public:
    typedef unsigned int Interrupt_Id;
    typedef void (* Interrupt_Handler)(Interrupt_Id);

    static const unsigned int UNSUPPORTED_INTERRUPT = ~1;

    enum {
        INT_UNKNOWN     = UNSUPPORTED_INTERRUPT,
        INT_SYS_TIMER   = UNSUPPORTED_INTERRUPT,
        INT_USR_TIMER   = UNSUPPORTED_INTERRUPT,
        INT_TIMER0      = UNSUPPORTED_INTERRUPT,
        INT_TIMER1      = UNSUPPORTED_INTERRUPT,
        INT_TIMER2      = UNSUPPORTED_INTERRUPT,
        INT_TIMER3      = UNSUPPORTED_INTERRUPT,
        INT_TIMER4      = UNSUPPORTED_INTERRUPT,
        INT_TIMER5      = UNSUPPORTED_INTERRUPT,
        INT_TIMER6      = UNSUPPORTED_INTERRUPT,
        INT_TIMER7      = UNSUPPORTED_INTERRUPT,
        INT_UART0       = UNSUPPORTED_INTERRUPT,
        INT_UART1       = UNSUPPORTED_INTERRUPT,
        INT_UART2       = UNSUPPORTED_INTERRUPT,
        INT_UART3       = UNSUPPORTED_INTERRUPT,
        INT_NIC0        = UNSUPPORTED_INTERRUPT,
        INT_NIC0_RX     = UNSUPPORTED_INTERRUPT,
        INT_NIC0_TX     = UNSUPPORTED_INTERRUPT,
        INT_NIC0_ERR    = UNSUPPORTED_INTERRUPT,
        INT_NIC0_TIMER  = UNSUPPORTED_INTERRUPT,
        INT_USB0        = UNSUPPORTED_INTERRUPT,
        INT_SPI0        = UNSUPPORTED_INTERRUPT,
        INT_I2C0        = UNSUPPORTED_INTERRUPT,
        INT_GPIOA       = UNSUPPORTED_INTERRUPT,
        INT_GPIOB       = UNSUPPORTED_INTERRUPT,
        INT_GPIOC       = UNSUPPORTED_INTERRUPT,
        INT_GPIOD       = UNSUPPORTED_INTERRUPT,
        INT_GPIOE       = UNSUPPORTED_INTERRUPT,
        INT_GPIOF       = UNSUPPORTED_INTERRUPT,
        INT_ADC0        = UNSUPPORTED_INTERRUPT
    };

protected:
    IC_Common() {}

public:
    static Interrupt_Handler int_vector(Interrupt_Id i);
    static void int_vector(Interrupt_Id i, const Interrupt_Handler & handler);
    static void int_vector(Interrupt_Id i, const Interrupt_Handler & int_handler, const Interrupt_Handler & eio_handler);

    static void enable();
    static void enable(Interrupt_Id i);
    static void disable();
    static void disable(Interrupt_Id i);

    static Interrupt_Id irq2int(Interrupt_Id i);       // Offset IRQs as seen by the bus to INTs seen by the CPU (if needed)
    static Interrupt_Id int2irq(Interrupt_Id i);       // Offset INTs as seen by the CPU to IRQs seen by the bus (if needed)

    static void ipi(unsigned int cpu, Interrupt_Id i); // Inter-processor Interrupt
};

__END_SYS

#endif

#if defined(__IC_H) && !defined(__ic_common_only__)
#include __IC_H
#endif
