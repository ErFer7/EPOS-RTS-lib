// EPOS Mutex Implementation

#include <synchronizer.h>

__BEGIN_SYS

Mutex::Mutex(): _locked(false)
{
    db<Synchronizer>(TRC) << "Mutex() => " << this << endl;

    Task::self()->enroll(this);
}


Mutex::~Mutex()
{
    db<Synchronizer>(TRC) << "~Mutex(this=" << this << ")" << endl;

    Task::self()->dismiss(this);
}


void Mutex::lock()
{
    db<Synchronizer>(TRC) << "Mutex::lock(this=" << this << ")" << endl;

    lock_for_acquiring();
    if(tsl(_locked))
        sleep();
    unlock_for_acquiring();
}


void Mutex::unlock()
{
    db<Synchronizer>(TRC) << "Mutex::unlock(this=" << this << ")" << endl;

    lock_for_releasing();
    if(_waiting.empty())
        _locked = false;
    else
        wakeup();
    unlock_for_releasing();
}

__END_SYS
