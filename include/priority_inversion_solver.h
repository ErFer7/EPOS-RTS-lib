// EPOS Priority Inversion Solver

#ifndef __priority_inversion_solver_h
#define __priority_inversion_solver_h

#include <architecture.h>
#include <process.h>

__BEGIN_SYS

class Priority_Inversion_Solver
{
    friend class Thread;
    friend class Mutex;
    friend class Semaphore;

public:
    typedef List<Priority_Inversion_Solver> PIS_List;
    typedef PIS_List::Element Element;

protected:
    typedef Thread::Criterion Criterion;

protected:
    Priority_Inversion_Solver(): _critical_section_thread(nullptr), _link(this) {}
    ~Priority_Inversion_Solver() { exit_critical_section(); }

    Criterion critical_section_priority() { return _critical_section_priority; }
    void critical_section_priority(Criterion p) { _critical_section_priority = p; }

    void enter_critical_section() {
        if (Thread::self()->priority() == Thread::MAIN || Thread::self()->priority() == Thread::IDLE)
            return;

        if (_critical_section_thread == Thread::self())  // TODO: Ver
            _critical_section_thread->synchronizers_in_use()->remove(this);

        _critical_section_thread = Thread::self();
        PIS_List * pis_list = _critical_section_thread->synchronizers_in_use();

        if (pis_list->empty())
            _critical_section_thread->save_original_priority();

        pis_list->insert(&_link);
        _critical_section_priority = Criterion(_critical_section_thread->priority());
    }

    void exit_critical_section() {
        if (!_critical_section_thread || Thread::self()->priority() == Thread::MAIN || Thread::self()->priority() == Thread::IDLE)
            return;

        PIS_List * pis_list = _critical_section_thread->synchronizers_in_use();
        pis_list->remove(this);

        if (pis_list->empty())
            _critical_section_thread->non_locked_priority(_critical_section_thread->original_priority());
        else
            _critical_section_thread->non_locked_priority(pis_list->tail()->object()->critical_section_priority());

        _critical_section_thread = nullptr;
    }

    void blocked() {
        if (!_critical_section_thread || Thread::self()->priority() == Thread::MAIN || Thread::self()->priority() == Thread::IDLE)
            return;

        Thread * blocked = Thread::self();
        Criterion blocked_priority = blocked->priority();

        if (blocked_priority < _critical_section_priority) {
            Criterion new_priority;

            if (Traits<Priority_Inversion_Solver>::priority_ceiling) 
                new_priority = Thread::ISR;
            else 
                new_priority = blocked_priority;

            _critical_section_priority = new_priority;

            Element * e = _link.next();
            while (e) {
                Priority_Inversion_Solver * resource = e->object();
                resource->critical_section_priority(new_priority);
                e = e->next();
            }

            _critical_section_thread->non_locked_priority(_critical_section_priority);
        }
    }

private:
    Thread * _critical_section_thread;
    Criterion _critical_section_priority;
    Element _link;
};

__END_SYS

#endif