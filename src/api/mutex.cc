// EPOS Mutex Implementation

#include <synchronizer.h>

__BEGIN_SYS

Mutex::Mutex(): _locked(false)
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
    priority_ceiling_check_lock();
    if(tsl(_locked))
        sleep();
    end_atomic();
}


void Mutex::unlock()
{
    db<Synchronizer>(TRC) << "Mutex::unlock(this=" << this << ")" << endl;

    begin_atomic();
    priority_ceiling_check_unlock();
    if(_queue.empty())
        _locked = false;
    else
        wakeup();
    end_atomic();
}

__END_SYS
