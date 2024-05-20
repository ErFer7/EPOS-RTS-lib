// EPOS Synchronizer Components

#ifndef __synchronizer_h
#define __synchronizer_h

#include <architecture.h>
#include <utility/handler.h>
#include <process.h>

__BEGIN_SYS

class Synchronizer_Common
{
protected:
    typedef Thread::Queue Queue;

protected:
    Synchronizer_Common() {}
    ~Synchronizer_Common() {
        Thread::lock();
        while(!_granted.empty()) {
            Queue::Element * e = _granted.remove();
            if(e)
                delete e;
        }
        if(!_waiting.empty())
            db<Synchronizer>(WRN) << "~Synchronizer(this=" << this << ") called with active blocked clients!" << endl;
        wakeup_all();
        Thread::unlock();
    }

    // Atomic operations
    bool tsl(volatile bool & lock) { return CPU::tsl(lock); }
    long finc(volatile long & number) { return CPU::finc(number); }
    long fdec(volatile long & number) { return CPU::fdec(number); }

    // Thread operations
    void lock_for_acquiring() { Thread::lock(); Thread::prioritize(&_granted); }
    void unlock_for_acquiring() { _granted.insert(new (SYSTEM) Queue::Element(Thread::running())); Thread::unlock(); }
    void lock_for_releasing() { Thread::lock(); Queue::Element * e = _granted.remove(); if(e) delete e; Thread::deprioritize(&_granted); Thread::deprioritize(&_waiting); }
    void unlock_for_releasing() { Thread::unlock(); }

    void sleep() { Thread::sleep(&_waiting); }
    void wakeup() { Thread::wakeup(&_waiting); }
    void wakeup_all() { Thread::wakeup_all(&_waiting); }

protected:
    Queue _waiting;
    Queue _granted;
};


class Mutex: protected Synchronizer_Common
{
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

private:
    volatile bool _locked;
};


class Semaphore: protected Synchronizer_Common
{
public:
    Semaphore(long v = 1);
    ~Semaphore();

    void p();
    void v();

private:
    volatile long _value;
};


// This is actually no Condition Variable
// check http://www.cs.duke.edu/courses/spring01/cps110/slides/sem/sld002.htm
class Condition: protected Synchronizer_Common
{
public:
    Condition();
    ~Condition();

    void wait();
    void signal();
    void broadcast();
};


// An event handler that triggers a mutex (see handler.h)
class Mutex_Handler: public Handler
{
public:
    Mutex_Handler(Mutex * h) : _handler(h) {}
    ~Mutex_Handler() {}

    void operator()() { _handler->unlock(); }

private:
    Mutex * _handler;
};

// An event handler that triggers a semaphore (see handler.h)
class Semaphore_Handler: public Handler
{
public:
    Semaphore_Handler(Semaphore * h) : _handler(h) {}
    ~Semaphore_Handler() {}

    void operator()() { _handler->v(); }

private:
    Semaphore * _handler;
};

// An event handler that triggers a condition variable (see handler.h)
class Condition_Handler: public Handler
{
public:
    Condition_Handler(Condition * h) : _handler(h) {}
    ~Condition_Handler() {}

    void operator()() { _handler->signal(); }

private:
    Condition * _handler;
};

__END_SYS

#endif
