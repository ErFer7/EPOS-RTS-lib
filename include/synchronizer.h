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

    virtual void sleep() { Thread::sleep(&_queue); }
    void wakeup() { Thread::wakeup(&_queue); }
    void wakeup_all() { Thread::wakeup_all(&_queue); }

protected:
    Queue _queue;
};


// TODO: Move the implementation to a source file
class Priority_Inheritance_Synchronizer: protected Synchronizer_Common
{
protected:
    typedef Thread::Criterion Criterion;
    typedef Thread::PIS_List::Element Element;

protected:
    Priority_Inheritance_Synchronizer(): _critical_section_thread(nullptr), _link(this) {}
    ~Priority_Inheritance_Synchronizer() {}

    Criterion critical_section_priority() { return _critical_section_priority; }

    void enter_critical_section() {
        _critical_section_thread = Thread::self();
        Thread::PIS_List * pis_list = _critical_section_thread->priority_inheritance_synchronizers();

        if (pis_list->empty())
            _critical_section_thread->save_original_priority();

        pis_list->insert(&_link);
        _critical_section_priority = Criterion(_critical_section_thread->priority());
    }

    void exit_critical_section() {
        // TODO: Handle the special case where the semaphore can be unlocked by other thread that isn't the running thread

        Thread::PIS_List * pis_list = _critical_section_thread->priority_inheritance_synchronizers();
        pis_list->remove(&_link);

        if (pis_list->empty())
            _critical_section_thread->non_locked_priority(_critical_section_thread->original_priority());
        else
            _critical_section_thread->non_locked_priority(pis_list->tail()->object()->critical_section_priority());

        _critical_section_thread = nullptr;
    }

    void sleep() {
        Thread * blocked = Thread::self();
        Criterion blocked_priority = blocked->priority();

        if (blocked_priority < _critical_section_priority) {
            _critical_section_priority = blocked_priority;
            _critical_section_thread->non_locked_priority(_critical_section_priority);
        }

        Synchronizer_Common::sleep();
    }

private:
    Thread * _critical_section_thread;
    Element _link;
    Criterion _critical_section_priority;
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
