// EPOS Timer Mediator Common Package

#ifndef __timer_h
#define __timer_h

#include <machine/ic.h>

__BEGIN_SYS

class Timer_Common
{
public:
    enum Channel : unsigned int {
        SCHEDULER,
        ALARM,
        USER,
        USER1 = USER,
        USER2,
        USER3,
        USER4,
        USER5
    };

    typedef long Tick;
    typedef IC_Common::Interrupt_Handler Handler;

protected:
    Timer_Common() {}

public:
    Tick read();
    Percent reset();

    void enable();
    void disable();

    PPB accuracy();
    Hertz frequency();
    void frequency(Hertz f);

    void handler(const Handler & handler);

    static Microsecond period(Hertz frequency) { return Microsecond(1000000) / Microsecond(frequency); }
    static Microsecond time(Tick ticks, Hertz frequency) { return Microsecond(ticks) * period(frequency); }
    static Tick ticks(Microsecond time, Hertz frequency) { return (time + period(frequency) / 2) / period(frequency); }
};

__END_SYS

#endif

#if defined(__TIMER_H) && !defined(__timer_common_only__)
#include __TIMER_H
#endif
