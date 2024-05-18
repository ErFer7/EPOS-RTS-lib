// EPOS RISC-V Timer Mediator Implementation

#include <machine/ic.h>
#include <machine/timer.h>

__BEGIN_SYS

Timer * Timer::_channels[CHANNELS];

void Timer::int_handler(Interrupt_Id i)
{
    if(_channels[ALARM] && (--_channels[ALARM]->_current <= 0)) {
        _channels[ALARM]->_current = _channels[ALARM]->_initial;
        _channels[ALARM]->_handler(i);
    }

    if(_channels[SCHEDULER] && (--_channels[SCHEDULER]->_current <= 0)) {
        _channels[SCHEDULER]->_current = _channels[SCHEDULER]->_initial;
        _channels[SCHEDULER]->_handler(i);
    }
}

__END_SYS
