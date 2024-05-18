// EPOS Semaphore Implementation

#include <synchronizer.h>

__BEGIN_SYS

Semaphore::Semaphore(long v) : _value(v)
{
    db<Synchronizer>(TRC) << "Semaphore(value=" << _value << ") => " << this << endl;
    
    Task::self()->enroll(this);
}


Semaphore::~Semaphore()
{
    db<Synchronizer>(TRC) << "~Semaphore(this=" << this << ")" << endl;

    Task::self()->dismiss(this);
}


void Semaphore::p()
{
    db<Synchronizer>(TRC) << "Semaphore::p(this=" << this << ",value=" << _value << ")" << endl;

    lock_for_acquiring();
    if(fdec(_value) < 1)
        sleep();
    unlock_for_acquiring();
}


void Semaphore::v()
{
    db<Synchronizer>(TRC) << "Semaphore::v(this=" << this << ",value=" << _value << ")" << endl;

    lock_for_releasing();
    if(finc(_value) < 0)
        wakeup();
    unlock_for_releasing();
}

__END_SYS
