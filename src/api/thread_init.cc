// EPOS Thread Initialization

#include <machine/timer.h>
#include <machine/ic.h>
#include <system.h>
#include <process.h>

__BEGIN_SYS

extern "C" { void __epos_app_entry(); }

void Thread::init()
{
    db<Init, Thread>(TRC) << "Thread::init()" << endl;

    // If EPOS is a library, then adjust the application entry point to __epos_app_entry, which will directly call main().
    // In this case, _init will have already been called, before Init_Application to construct MAIN's global objects.
    Thread::_running = new (kmalloc(sizeof(Thread))) Thread(Thread::Configuration(Thread::RUNNING, Thread::NORMAL), reinterpret_cast<int (*)()>(__epos_app_entry));

    _timer = new (kmalloc(sizeof(Scheduler_Timer))) Scheduler_Timer(QUANTUM, time_slicer);

    // No more interrupts until we reach init_end
    CPU::int_disable();

    // Transition from CPU-based locking to thread-based locking
    _not_booting = true;
}

__END_SYS
