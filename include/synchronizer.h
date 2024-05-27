// EPOS Synchronizer Components

#ifndef __synchronizer_h
#define __synchronizer_h

#include <architecture.h>
#include <utility/handler.h>
#include <process.h>

__BEGIN_SYS

extern OStream kout;

class Synchronizer_Common
{
    friend class Thread;

protected:
    typedef Thread::Criterion Criterion;

    struct Thread_Priority {
        Thread_Priority(Thread * t, Criterion p): thread(t), priority(p) {}

        Thread * thread;
        Criterion priority;
    };

    typedef Queue<Thread_Priority> Priority_Queue;
    typedef Thread::Thread_Queue Thread_Queue;

protected:
    Synchronizer_Common(bool priority_inversion = true): _solve_priority_inversion(priority_inversion), _priority_raised(false) {}
    ~Synchronizer_Common() {
        Thread::lock();
        while(!_granted.empty()) {
            Thread_Queue::Element * e = _granted.remove();
            if(e) {
                Thread * thread = e->object();
                thread->release_synchronizer(this);

                delete e;
            }
        }
        if(!_waiting.empty())
            db<Synchronizer>(WRN) << "~Synchronizer(this=" << this << ") called with active blocked clients!" << endl;
        wakeup_all();
        Thread::unlock();
    }

    // Atomic operations
    bool tsl(volatile int & lock) { return CPU::tsl(lock); }
    void asz(volatile int & lock) { CPU::asz(lock); }
    long finc(volatile long & number) { return CPU::finc(number); }
    long fdec(volatile long & number) { return CPU::fdec(number); }

    // Thread operations
    void lock_for_acquiring() { Thread::lock(); }

    void unlock_for_acquiring() {
        if (_solve_priority_inversion) {
            _granted.insert(new (SYSTEM) Thread_Queue::Element(Thread::running()));
            Thread::acquire_synchronizer(this);
        }

        Thread::unlock();
    }

    void lock_for_releasing() {
        Thread::lock();

        if (_solve_priority_inversion) {
            Thread_Queue::Element * e = _granted.remove();
            if(e) delete e;
            Thread::release_synchronizer(this);
        }
    }

    void unlock_for_releasing() { Thread::unlock(); }

    void sleep() {
        Thread::handle_synchronizer_blocking(this);
        Thread::sleep(&_waiting);
    }

    void wakeup() { Thread::wakeup(&_waiting); }

    void wakeup_all() { Thread::wakeup_all(&_waiting); }

    Thread_Queue * waiting() { return &_waiting; }
    Thread_Queue * granted() { return &_granted; }

    void save_thread_priority(Thread * t, Criterion p) {
        _priorities.insert(new (SYSTEM) Priority_Queue::Element(new (SYSTEM) Thread_Priority(t, p)));
    }

    Criterion get_saved_thread_priority(Thread * t) {
        for (Priority_Queue::Element * e = _priorities.head(); e; e = e->next()) {
            if (e->object()->thread == t) {
                Criterion priority = e->object()->priority;
                _priorities.remove(e);

                delete e->object();
                delete e;

                return priority;
            }
        }

        return Thread::Criterion::NORMAL;
    }

    Criterion priority() { return _priority; }
    void priority(Criterion c) { _priority = c; }

    void priority_raised(bool priority_raised) { _priority_raised = true; }
    bool priority_raised() { return _priority_raised; }

protected:
    Thread_Queue _waiting;
    Thread_Queue _granted;
    Priority_Queue _priorities;  // Store the priorities of threads that are granted, this is used to restore the priorities correctly
    Criterion _priority;         // Current inherited or ceiled priority
    bool _solve_priority_inversion;
    bool _priority_raised;
};


class Mutex: protected Synchronizer_Common
{
public:
    Mutex(bool priority_inversion = true);
    ~Mutex();

    void lock();
    void unlock();

private:
    volatile int _locked;
};


class Semaphore: protected Synchronizer_Common
{
public:
    Semaphore(long v = 1, bool priority_inversion = true);
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
    Condition(bool priority_inversion = true);
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
