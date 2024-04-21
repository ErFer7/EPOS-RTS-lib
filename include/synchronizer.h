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
    ~Synchronizer_Common() { begin_atomic(); wakeup_all(); end_atomic(); }

    // Atomic operations
    bool tsl(volatile bool & lock) { return CPU::tsl(lock); }
    long finc(volatile long & number) { return CPU::finc(number); }
    long fdec(volatile long & number) { return CPU::fdec(number); }

    // Thread operations
    void begin_atomic() { Thread::lock(); }
    void end_atomic() { Thread::unlock(); }

    // For the team: Virtual here specifies that the function can be overriden by a subclass (such as Priority_Inheritance_Synchronizer)
    virtual void sleep() { Thread::sleep(&_queue); }
    virtual void wakeup() { Thread::wakeup(&_queue); }
    virtual void wakeup_all() { Thread::wakeup_all(&_queue); }

protected:
    Queue _queue;
};


// TODO: Move the implementation to a source file
class Priority_Inheritance_Synchronizer: protected Synchronizer_Common
{
protected:
    typedef Thread::Criterion Criterion;
public:
    Criterion critical_section_priority() { return _critical_section_priority; }
protected:
    // TODO: Check if this is ok
    Priority_Inheritance_Synchronizer():
        _critical_section_thread(nullptr),
        _prev_priority_inheritance_synchronizer(nullptr) {}
    ~Priority_Inheritance_Synchronizer() {}

    void enter_critical_section() {
        if (_critical_section_thread) {
            if (Thread::self() == _critical_section_thread)  // If the same thread is calling p() twice or more
                return;

            exit_critical_section(false);
        }

        _critical_section_thread = Thread::self();
        _prev_priority_inheritance_synchronizer = _critical_section_thread->priority_inheritance_synchronizer();
        _critical_section_thread->priority_inheritance_synchronizer(this);

        _critical_section_priority = Criterion(_critical_section_thread->priority());  // TODO: Check if this is ok
        _original_priority = _critical_section_priority;
    }

    void exit_critical_section(bool unlocking = true) {
        if (!_critical_section_thread && unlocking && Thread::self() != _critical_section_thread)
            return;

        if (_prev_priority_inheritance_synchronizer)
            _critical_section_thread->priority(_prev_priority_inheritance_synchronizer->critical_section_priority());
        else
            _critical_section_thread->priority(_original_priority);

        _critical_section_thread->priority_inheritance_synchronizer(_prev_priority_inheritance_synchronizer);
        _critical_section_thread = nullptr;
    }

    void sleep() {
        Synchronizer_Common::sleep();
        solve_priority();
    }

    void wakeup() {
        Synchronizer_Common::wakeup();
        enter_critical_section();
        solve_priority();
    }

    // Maybe we will need a special method just for the wakeup_all()
private:
    void solve_priority() {
        // Since the queue is ordered by priority, the highest priority thread will be the first one
        if (!_queue.empty()) {
            Criterion highest_priority = _queue.head()->object()->priority();

            if (_critical_section_priority > highest_priority) {
                _critical_section_priority = highest_priority;
                _critical_section_thread->priority(_critical_section_priority);
            }
        }
    }

private:
    Thread * _critical_section_thread;

private:
    Criterion _critical_section_priority;
    Criterion _original_priority;
    Priority_Inheritance_Synchronizer * _prev_priority_inheritance_synchronizer;
};


class Mutex: protected Priority_Inheritance_Synchronizer
{
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

private:
    volatile bool _locked;
};


class Semaphore: protected Priority_Inheritance_Synchronizer
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
