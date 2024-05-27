// EPOS Thread Implementation

#include <boot_synchronizer.h>
#include <machine.h>
#include <system.h>
#include <process.h>
#include <synchronizer.h>

extern "C" { volatile unsigned long _running() __attribute__ ((alias ("_ZN4EPOS1S6Thread4selfEv"))); }

__BEGIN_SYS

extern OStream kout;

bool Thread::_not_booting;
volatile unsigned int Thread::_thread_count;
Scheduler_Timer * Thread::_timer;
Scheduler<Thread> Thread::_scheduler;
Core_Spin Thread::_lock;


void Thread::constructor_prologue(unsigned int stack_size)
{
    lock();

    _thread_count++;
    _scheduler.insert(this);

    _stack = new (SYSTEM) char[stack_size];
    _acquired_synchronizers = new Synchronizer_Queue;
    _waiting = new Thread_Queue;
}


void Thread::constructor_epilogue(Log_Addr entry, unsigned int stack_size)
{
    db<Thread>(TRC) << "Thread(entry=" << entry
                    << ",state=" << _state
                    << ",priority=" << _link.rank()
                    << ",stack={b=" << reinterpret_cast<void *>(_stack)
                    << ",s=" << stack_size
                    << "},context={b=" << _context
                    << "," << *_context << "}) => " << this << endl;

    assert((_state != WAITING) && (_state != FINISHING)); // invalid states

    if(_link.rank() != IDLE)
        _task->enroll(this);

    if((_state != READY) && (_state != RUNNING))
        _scheduler.suspend(this);

    criterion().handle(Criterion::CREATE);

    if(preemptive && (_state == READY) && (_link.rank() != IDLE))
        reschedule(_link.rank().queue());

    unlock();
}


Thread::~Thread()
{
    lock();

    db<Thread>(TRC) << "~Thread(this=" << this
                    << ",state=" << _state
                    << ",priority=" << _link.rank()
                    << ",stack={b=" << reinterpret_cast<void *>(_stack)
                    << ",context={b=" << _context
                    << "," << *_context << "})" << endl;

    // The running thread cannot delete itself!
    assert(_state != RUNNING);

    switch(_state) {
    case RUNNING:  // For switch completion only: the running thread would have deleted itself! Stack wouldn't have been released!
        exit(-1);
        break;
    case READY:
        _scheduler.remove(this);
        _thread_count--;
        break;
    case SUSPENDED:
        _scheduler.resume(this);
        _scheduler.remove(this);
        _thread_count--;
        break;
    case WAITING:
        _waiting->remove(this);
        _scheduler.resume(this);
        _scheduler.remove(this);
        _thread_count--;
        break;
    case FINISHING: // Already called exit()
        break;
    }

    _task->dismiss(this);

    if(_joining)
        _joining->resume();

    unlock();

    delete _acquired_synchronizers;
    delete _waiting;
    delete _stack;
}


void Thread::priority(Criterion c)
{
    lock();
    update_priority(c);
    unlock();
}


int Thread::join()
{
    lock();

    db<Thread>(TRC) << "Thread::join(this=" << this << ",state=" << _state << ")" << endl;

    // Precondition: no Thread::self()->join()
    assert(running() != this);

    // Precondition: a single joiner
    assert(!_joining);

    if(_state != FINISHING) {
        Thread * prev = running();

        _joining = prev;
        prev->_state = SUSPENDED;
        _scheduler.suspend(prev); // implicitly choose() if suspending chosen()

        Thread * next = _scheduler.chosen();

        dispatch(prev, next);
    }

    unlock();

    return *reinterpret_cast<int *>(_stack);
}


void Thread::pass()
{
    lock();

    db<Thread>(TRC) << "Thread::pass(this=" << this << ")" << endl;

    Thread * prev = running();
    Thread * next = _scheduler.choose(this);

    if(next)
        dispatch(prev, next, false);
    else
        db<Thread>(WRN) << "Thread::pass => thread (" << this << ") not ready!" << endl;

    unlock();
}


void Thread::suspend()
{
    lock();

    db<Thread>(TRC) << "Thread::suspend(this=" << this << ")" << endl;

    Thread * prev = running();

    _state = SUSPENDED;
    _scheduler.suspend(this);

    Thread * next = _scheduler.chosen();

    dispatch(prev, next);

    unlock();
}


void Thread::resume()
{
    lock();

    db<Thread>(TRC) << "Thread::resume(this=" << this << ")" << endl;

    if(_state == SUSPENDED) {
        _state = READY;
        _scheduler.resume(this);

        if(preemptive) {
            if (Criterion::core_scheduling == Criterion::GLOBAL_MULTICORE) {
                for (unsigned long i = 0; i < Traits<Build>::CPUS; i++)
                    reschedule(i);
            } else
                reschedule(_link.rank().queue());
        }
    } else
        db<Thread>(WRN) << "Resume called for unsuspended object!" << endl;

    unlock();
}


Thread * volatile Thread::self() {
    return _not_booting ? running() : reinterpret_cast<Thread * volatile>(CPU::id() + 1);
}


void Thread::yield()
{
    lock();

    db<Thread>(TRC) << "Thread::yield(running=" << running() << ")" << endl;

    Thread * prev = running();
    Thread * next = _scheduler.choose_another();

    dispatch(prev, next);

    unlock();
}


void Thread::exit(int status)
{
    lock();

    db<Thread>(TRC) << "Thread::exit(status=" << status << ") [running=" << running() << "]" << endl;

    Thread * prev = running();
    _scheduler.remove(prev);
    prev->_state = FINISHING;
    *reinterpret_cast<int *>(prev->_stack) = status;
    prev->criterion().handle(Criterion::FINISH);

    _thread_count--;

    if(prev->_joining) {
        prev->_joining->_state = READY;
        _scheduler.resume(prev->_joining);
        prev->_joining = 0;
    }

    Thread * next = _scheduler.choose(); // at least idle will always be there

    dispatch(prev, next);

    unlock();
}


void Thread::sleep(Thread_Queue * q)
{
    db<Thread>(TRC) << "Thread::sleep(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller

    Thread * prev = running();
    _scheduler.suspend(prev);
    prev->_state = WAITING;
    prev->_waiting = q;
    q->insert(&prev->_link);

    Thread * next = _scheduler.chosen();

    dispatch(prev, next);
}


void Thread::wakeup(Thread_Queue * q)
{
    db<Thread>(TRC) << "Thread::wakeup(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller

    if(!q->empty()) {
        Thread * t = q->remove()->object();
        t->_state = READY;
        t->_waiting = 0;
        _scheduler.resume(t);

        if(preemptive) {
            if (Criterion::core_scheduling == Criterion::GLOBAL_MULTICORE) {
                for (unsigned long i = 0; i < Traits<Build>::CPUS; i++)
                    reschedule(i);
            } else
                reschedule(t->_link.rank().queue());
        }
    }
}


void Thread::wakeup_all(Thread_Queue * q)
{
    db<Thread>(TRC) << "Thread::wakeup_all(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller

    if(!q->empty()) {
        while(!q->empty()) {
            Thread * t = q->remove()->object();
            t->_state = READY;
            t->_waiting = 0;
            _scheduler.resume(t);
        }

        if(preemptive) {
            for(unsigned long i = 0; i < Traits<Build>::CPUS; i++)
                reschedule(i);
        }
    }
}

void Thread::acquire_synchronizer(Synchronizer_Common * synchronizer) {
    db<Thread>(TRC) << "Thread::acquire_resource(synchronizer=" << synchronizer << ") [running=" << running() << "]" << endl;

    assert(locked()); // locking handled by caller

    if(priority_inversion_protocol == Traits<Build>::NONE)
        return;

    Thread * running = Thread::running();
    Criterion running_priority = running->criterion();

    if (running_priority == MAIN)
        return;

    running->_acquired_synchronizers->insert(new (SYSTEM) Synchronizer_Queue::Element(synchronizer));
    synchronizer->save_thread_priority(running, running_priority);
}

void Thread::release_synchronizer(Synchronizer_Common * synchronizer) {
    assert(locked()); // locking handled by caller

    if(priority_inversion_protocol == Traits<Build>::NONE)
        return;

    Thread_Queue * granted = synchronizer->granted();

    db<Thread>(TRC) << "Thread::release_resource(q=" << granted << ") [running=" << running() << "]" << endl;

    Thread * running = Thread::running();

    synchronizer->priority_raised(false);

    Synchronizer_Queue::Element * removed = running->_acquired_synchronizers->remove(synchronizer);

    Criterion max = IDLE;

    for (Synchronizer_Queue::Element * e = removed->prev(); e; e = e->prev()) {
        Synchronizer_Common * previours_synchronizer = e->object();

        if (previours_synchronizer->priority_raised() && previours_synchronizer->priority() < max)
            max = previours_synchronizer->priority();
    }

    if (max != IDLE) {
        for (Thread_Queue::Element * e = granted->head(); e; e = e->next()) {
            Thread * thread = e->object();
            Criterion priority = thread->criterion();

            if (priority != MAIN && max < priority)
                thread->update_priority(max);
        }
        running->update_priority(max);
    } else {
        for (Thread_Queue::Element * e = granted->head(); e; e = e->next()) {
            Thread * thread = e->object();
            Criterion priority = thread->criterion();

            if (priority != MAIN)
                synchronizer->get_saved_thread_priority(thread);
        }
        running->update_priority(synchronizer->get_saved_thread_priority(running));
    }

    delete removed;
}

void Thread::handle_synchronizer_blocking(Synchronizer_Common * synchronizer) {
    assert(locked()); // locking handled by caller

    if(priority_inversion_protocol == Traits<Build>::NONE)
        return;

    Thread_Queue * granted = synchronizer->granted();

    db<Thread>(TRC) << "Thread::blocked_by_resource(q=" << granted << ") [running=" << running() << "]" << endl;

    Criterion blocked_thread_priority = Thread::running()->criterion();

    if (!granted->empty()) {
        Criterion new_priority = (priority_inversion_protocol == Traits<Build>::CEILING) ? CEILING : blocked_thread_priority;

        for (Thread_Queue::Element * e = granted->head(); e; e = e->next()) {
            Thread * critical_section_thread = e->object();
            Criterion critical_section_thread_priority = critical_section_thread->criterion();

            if (critical_section_thread_priority != MAIN && new_priority < critical_section_thread->criterion()) {
                synchronizer->priority(new_priority);
                synchronizer->priority_raised(true);
                critical_section_thread->update_priority(new_priority);
            }

            Synchronizer_Queue * acquired_synchronizers = critical_section_thread->acquired_synchronizers();

            for (Synchronizer_Queue::Element * e = acquired_synchronizers->head(); e; e = e->next()) {
                Synchronizer_Common * synchronizer = e->object();
                Thread_Queue * granted = synchronizer->granted();

                for (Thread_Queue::Element * e = granted->head(); e; e = e->next()) {
                    Thread * thread = e->object();
                    Criterion priority = thread->criterion();

                    if (priority != MAIN && new_priority < priority)
                        thread->update_priority(new_priority);
                }
            }
        }
    }
}

void Thread::update_priority(Criterion c) {
    db<Thread>(TRC) << "Thread::update_priority(this=" << this << ",prio=" << c << ")" << endl;

    assert(locked());

    if(_state != RUNNING) { // reorder the scheduling queue
        _scheduler.suspend(this);
        _link.rank(c);
        _scheduler.resume(this);
    } else
        _link.rank(c);

    if(preemptive) {
        if (Criterion::core_scheduling == Criterion::GLOBAL_MULTICORE) {
            for (unsigned long i = 0; i < Traits<Build>::CPUS; i++)
                reschedule(i);
        } else
            reschedule(_link.rank().queue());
    }
}

void Thread::reschedule()
{
    if(!Criterion::timed || Traits<Thread>::hysterically_debugged)
        db<Thread>(TRC) << "Thread::reschedule()" << endl;

    assert(locked()); // locking handled by caller

    Thread * prev = running();
    Thread * next = prev;

    if (Criterion::core_scheduling == Criterion::GLOBAL_MULTICORE) {
        if (!(prev->priority() == IDLE && _scheduler.head() && _scheduler.head()->object()->priority() == IDLE))
            next = _scheduler.choose();
    } else
        next = _scheduler.choose();

    dispatch(prev, next);
}


void Thread::reschedule(unsigned int cpu)
{
    assert(locked()); // locking handled by caller

    if(Criterion::core_scheduling == Criterion::SINGLECORE || CPU::id() == cpu)
        reschedule();
    else {
        db<Thread>(TRC) << "Thread::reschedule(cpu=" << cpu << ")" << endl;
        IC::ipi(cpu, IC::INT_RESCHEDULER);
    }
}


void Thread::rescheduler(IC::Interrupt_Id i)
{
    lock();
    reschedule();
    unlock();
}


void Thread::time_slicer(IC::Interrupt_Id i)
{
    lock();
    reschedule();
    unlock();
}


void Thread::dispatch(Thread * prev, Thread * next, bool charge)
{
    // "next" is not in the scheduler's queue anymore. It's already "chosen"

    if(charge && Criterion::timed)
        _timer->restart();

    if(prev != next) {
        if(Criterion::dynamic) {
            prev->criterion().handle(Criterion::CHARGE | Criterion::LEAVE);
            for_all_threads(Criterion::UPDATE);
            next->criterion().handle(Criterion::AWARD  | Criterion::ENTER);
        }

        if(prev->_state == RUNNING)
            prev->_state = READY;
        next->_state = RUNNING;

        db<Thread>(TRC) << "Thread::dispatch(prev=" << prev << ",next=" << next << ")" << endl;
        if(Traits<Thread>::debugged && Traits<Debug>::info) {
            CPU::Context tmp;
            tmp.save();
            db<Thread>(INF) << "Thread::dispatch:prev={" << prev << ",ctx=" << tmp << "}" << endl;
        }
        db<Thread>(INF) << "Thread::dispatch:next={" << next << ",ctx=" << *next->_context << "}" << endl;

        _lock.release(false);

        // The non-volatile pointer to volatile pointer to a non-volatile context is correct
        // and necessary because of context switches, but here, we are locked() and
        // passing the volatile to switch_constext forces it to push prev onto the stack,
        // disrupting the context (it doesn't make a difference for Intel, which already saves
        // parameters on the stack anyway).
        CPU::switch_context(const_cast<Context **>(&prev->_context), next->_context);

        _lock.acquire();
    }
}


int Thread::idle()
{
    db<Thread>(TRC) << "Thread::idle(this=" << running() << ")" << endl;

    while(_thread_count > CPU::cores()) { // someone else besides idle
        if(Traits<Thread>::trace_idle)
            db<Thread>(TRC) << "Thread::idle(this=" << running() << ")" << endl;

        CPU::int_enable();
        CPU::halt();

        if(!preemptive)
            yield();
    }

    if (Boot_Synchronizer::acquire_single_core_section()) {
        kout << "\n\n*** The last thread under control of EPOS has finished." << endl;
        kout << "*** EPOS is shutting down!" << endl;
    }

    Machine::reboot();

    return 0;
}

__END_SYS
