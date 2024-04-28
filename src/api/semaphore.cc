// EPOS Semaphore Implementation

#include <synchronizer.h>

__BEGIN_SYS

static OStream kout;

Semaphore::Semaphore(long v) : _value(v)
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
        _pis.blocked();
        sleep();
    }

    if (_value == 0)
        _pis.enter_critical_section();
    end_atomic();
}


void Semaphore::v()
{
    db<Synchronizer>(TRC) << "Semaphore::v(this=" << this << ",value=" << _value << ")" << endl;

    begin_atomic();
    if(finc(_value) < 0) {
        _pis.exit_critical_section();
        wakeup();
    }
    end_atomic();
}

__END_SYS
