// EPOS Mutex Implementation

#include <synchronizer.h>

__BEGIN_SYS

Mutex::Mutex(bool solve_priority_inversion): _locked(false), _solve_priority_inversion(solve_priority_inversion)
{
    db<Synchronizer>(TRC) << "Mutex() => " << this << endl;
}


Mutex::~Mutex()
{
    db<Synchronizer>(TRC) << "~Mutex(this=" << this << ")" << endl;
}


void Mutex::lock()
{
    db<Synchronizer>(TRC) << "Mutex::lock(this=" << this << ")" << endl;

    begin_atomic();
    if(tsl(_locked)) {
        if (_solve_priority_inversion)
            _pis.blocked();
        sleep();
    }
    if (_solve_priority_inversion)
        _pis.enter_critical_section();
    end_atomic();
}


void Mutex::unlock()
{
    db<Synchronizer>(TRC) << "Mutex::unlock(this=" << this << ")" << endl;

    begin_atomic();
    if (_solve_priority_inversion)
        _pis.exit_critical_section();

    if(_queue.empty())
        _locked = false;
    else
        wakeup();
    end_atomic();
}

__END_SYS
