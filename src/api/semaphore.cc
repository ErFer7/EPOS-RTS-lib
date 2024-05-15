// EPOS Semaphore Implementation

#include <synchronizer.h>

__BEGIN_SYS

Semaphore::Semaphore(long v, bool solve_priority_inversion) : _value(v), _solve_priority_inversion(solve_priority_inversion)
{
    db<Synchronizer>(TRC) << "Semaphore(value=" << _value << ") => " << this << endl;
}


Semaphore::~Semaphore()
{
    db<Synchronizer>(TRC) << "~Semaphore(this=" << this << ")" << endl;
}


void Semaphore::p()
{
    db<Synchronizer>(TRC) << "Semaphore::p(this=" << this << ",value=" << _value << ")" << endl;

    begin_atomic();
    if(fdec(_value) < 1) {
        if (_solve_priority_inversion)
            _pis.blocked();
        sleep();
    }

    if (_solve_priority_inversion && _value == 0)
        _pis.acquire_resource();
    end_atomic();
}


void Semaphore::v()
{
    db<Synchronizer>(TRC) << "Semaphore::v(this=" << this << ",value=" << _value << ")" << endl;

    begin_atomic();
    if(finc(_value) < 0) {
        if (_solve_priority_inversion)
            _pis.release_resource();
        wakeup();
    }
    end_atomic();
}

__END_SYS
