// EPOS Priority Inversion Solver Declarations

#ifndef __priority_inversion_solver_h
#define __priority_inversion_solver_h

#include <architecture.h>
#include <process.h>

__BEGIN_SYS

class Priority_Inversion_Solver
{
    friend class Mutex;
    friend class Semaphore;

public:
    typedef List<Priority_Inversion_Solver> PIS_List;
    typedef PIS_List::Element Element;

protected:
    typedef Thread::Criterion Criterion;

protected:
    Priority_Inversion_Solver();
    ~Priority_Inversion_Solver();

    Criterion critical_section_priority() { return _critical_section_priority; }
    void critical_section_priority(Criterion p) { _critical_section_priority = p; }

    void enter_critical_section();
    void exit_critical_section();
    void blocked();

private:
    Thread * _critical_section_thread;
    Criterion _critical_section_priority;
    Element _link;
};

__END_SYS

#endif