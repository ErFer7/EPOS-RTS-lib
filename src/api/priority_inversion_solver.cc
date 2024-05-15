// EPOS Priority Inversion Solver Implementation

#include <priority_inversion_solver.h>

__BEGIN_SYS

Priority_Inversion_Solver::Priority_Inversion_Solver(): _critical_section_thread(nullptr), _link(this) {
    db<Priority_Inversion_Solver>(TRC) << "Priority_Inversion_Solver() => " << this << endl;
}


Priority_Inversion_Solver::~Priority_Inversion_Solver() {
    db<Priority_Inversion_Solver>(TRC) << "~Priority_Inversion_Solver(this=" << this << ")" << endl;
}


void Priority_Inversion_Solver::acquire_resource() {
    db<Priority_Inversion_Solver>(TRC) << "Priority_Inversion_Solver::enter_critical_section(this=" << this << ")" << endl;

    Thread * self = Thread::self();

    if (self->priority() == Thread::MAIN || self->priority() == Thread::IDLE)
        return;

    if (_critical_section_thread)  // This will be handled to avoid any serious problems
        _critical_section_thread->synchronizers_in_use()->remove(&_link);

    _critical_section_thread = self;
    PIS_List * pis_list = _critical_section_thread->synchronizers_in_use();

    if (pis_list->empty())
        _critical_section_thread->save_original_priority();

    pis_list->insert(&_link);
    _critical_section_priority = _critical_section_thread->original_priority();
}


void Priority_Inversion_Solver::release_resource() {
    db<Priority_Inversion_Solver>(TRC) << "Priority_Inversion_Solver::exit_critical_section(this=" << this << ")" << endl;

    Criterion self_priority = Thread::self()->priority();

    if (!_critical_section_thread || self_priority == Thread::MAIN || self_priority == Thread::IDLE)
        return;

    PIS_List * pis_list = _critical_section_thread->synchronizers_in_use();
    pis_list->remove(&_link);

    if (pis_list->empty())
        _critical_section_thread->non_locked_priority(_critical_section_thread->original_priority());
    else {
        Criterion max_priority = Thread::IDLE;

        for (Element * e = pis_list->head(); e; e = e->next()) {
            Criterion priority = e->object()->critical_section_priority();

            if (priority < max_priority)
                max_priority = priority;
        }

        _critical_section_thread->non_locked_priority(max_priority);
    }

    _critical_section_thread = nullptr;
    _critical_section_priority = Thread::IDLE;
}


void Priority_Inversion_Solver::blocked() {
    db<Priority_Inversion_Solver>(TRC) << "Priority_Inversion_Solver::blocked(this=" << this << ")" << endl;

    if (!_critical_section_thread || Thread::self()->priority() == Thread::MAIN || Thread::self()->priority() == Thread::IDLE)
        return;

    Thread * blocked = Thread::self();
    Criterion blocked_priority = blocked->priority();

    if (blocked_priority < _critical_section_thread->priority()) {
        Criterion new_priority;

        if (Traits<Priority_Inversion_Solver>::priority_ceiling)
            new_priority = Thread::ISR;
        else
            new_priority = blocked_priority;

        _critical_section_priority = new_priority;
        _critical_section_thread->non_locked_priority(_critical_section_priority);
    }
}

__END_SYS