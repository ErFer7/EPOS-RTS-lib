// EPOS RISC-V Timer Mediator Implementation

#include <machine/ic.h>
#include <machine/timer.h>

__BEGIN_SYS

unsigned int Timer::_alarm_handler_cpu;
Timer * Timer::_channels[CHANNELS];

void Timer::int_handler(Interrupt_Id i)
{
    if(_channels[ALARM] && CPU::id() == _alarm_handler_cpu && (--_channels[ALARM]->_current[0] <= 0)) {
        _channels[ALARM]->_current[_alarm_handler_cpu] = _channels[ALARM]->_initial;
        _channels[ALARM]->_handler(i);
    }

    if(_channels[SCHEDULER] && (--_channels[SCHEDULER]->_current[CPU::id()] <= 0)) {
        _channels[SCHEDULER]->_current[CPU::id()] = _channels[SCHEDULER]->_initial;
        _channels[SCHEDULER]->_handler(i);
    }
}

__END_SYS
