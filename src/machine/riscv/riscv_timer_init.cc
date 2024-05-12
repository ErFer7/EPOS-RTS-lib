// EPOS RISC-V Timer Mediator Initialization

#include <architecture/cpu.h>
#include <boot_synchronizer.h>
#include <machine/timer.h>
#include <machine/ic.h>

__BEGIN_SYS

void Timer::init()
{
    db<Init, Timer>(TRC) << "Timer::init()" << endl;

    assert(CPU::int_disabled());

    IC::int_vector(IC::INT_SYS_TIMER, int_handler);

    if (Boot_Synchronizer::try_acquire()) {
        _alarm_handler_cpu = CPU::id();
    }

    reset();
    IC::enable(IC::INT_SYS_TIMER);
}

__END_SYS
