// EPOS Cortex Timer Mediator Initialization

#include <machine/timer.h>
#include <machine/ic.h>

__BEGIN_SYS

void Timer::init()
{
    db<Init, Timer>(TRC) << "Timer::init()" << endl;

    Engine::init();
    IC::int_vector(IC::INT_SYS_TIMER, int_handler, eoi);
    IC::enable(IC::INT_SYS_TIMER);
}

__END_SYS
