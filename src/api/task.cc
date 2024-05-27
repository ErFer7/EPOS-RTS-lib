// EPOS Task Implementation

#include <process.h>
#include <synchronizer.h>
#include <time.h>
#include <memory.h>

__BEGIN_SYS

Task * volatile Task::_current;

Task::~Task()
{
    db<Task>(TRC) << "~Task(this=" << this << ")" << endl;

    while(!_resources.empty()) {
        Resource * r = _resources.remove();
        switch(r->type()) {
        case Type<Thread>::ID:
            db<Task>(INF) << "~Task: deleting Thread " << r->object() << "!" << endl;
            delete reinterpret_cast<Thread *>(r->object());
            break;
        case Type<Mutex>::ID:
            db<Task>(INF) << "~Task: deleting Mutex " << r->object() << "!" << endl;
            delete reinterpret_cast<Mutex *>(r->object());
            break;
        case Type<Semaphore>::ID:
            db<Task>(INF) << "~Task: deleting Semaphore " << r->object() << "!" << endl;
            delete reinterpret_cast<Semaphore *>(r->object());
            break;
        case Type<Condition>::ID:
            db<Task>(INF) << "~Task: deleting Condition " << r->object() << "!" << endl;
            delete reinterpret_cast<Condition *>(r->object());
            break;
        case Type<Alarm>::ID:
            db<Task>(INF) << "~Task: deleting Alarm " << r->object() << "!" << endl;
            delete reinterpret_cast<Alarm *>(r->object());
            break;
        case Type<Segment>::ID:
            db<Task>(INF) << "~Task: deleting Segment " << r->object() << "!" << endl;
            delete reinterpret_cast<Segment *>(r->object());
            break;
        default:
            db<Task>(WRN) << "Unknown resource in task's resource list. Memory released, but no destructor was called!" << endl;
            delete reinterpret_cast<char *>(r->object());
        }
    }
}

__END_SYS
