// EPOS Thread Initialization

#include <boot_synchronizer.h>
#include <machine/frequency_profiler.h>
#include <machine/timer.h>
#include <machine/ic.h>
#include <system.h>
#include <process.h>

__BEGIN_SYS

extern "C" { void __epos_app_entry(); }

void Thread::init()
{
    db<Init, Thread>(TRC) << "Thread::init()" << endl;

    Criterion::init();

    if (Boot_Synchronizer::try_acquire()) {
        typedef int (Main)();

        // If EPOS is a library, then adjust the application entry point to __epos_app_entry, which will directly call main().
        // In this case, _init will have already been called, before Init_Application to construct MAIN's global objects.
        Main * main = reinterpret_cast<Main *>(__epos_app_entry);

        new (SYSTEM) Thread(Thread::Configuration(Thread::RUNNING, Thread::MAIN), main);
    }

    // Idle thread creation does not cause rescheduling (see Thread::constructor_epilogue)
    new (SYSTEM) Thread(Thread::Configuration(Thread::READY, Thread::IDLE), &Thread::idle);

    CPU::smp_barrier();  // TODO: Check this

    // The installation of the scheduler timer handler does not need to be done after the
    // creation of threads, since the constructor won't call reschedule() which won't call
    // dispatch that could call timer->reset()
    // Letting reschedule() happen during thread creation is also harmless, since MAIN is
    // created first and dispatch won't replace it nor by itself neither by IDLE (which
    // has a lower priority)
    if(Criterion::timed && Boot_Synchronizer::try_acquire())
        _timer = new (SYSTEM) Scheduler_Timer(QUANTUM, time_slicer);

    if (Traits<Frequency_Profiler>::profiled && Boot_Synchronizer::try_acquire())
        Frequency_Profiler::profile();

    // No more interrupts until we reach init_end
    CPU::int_disable();

    if (Traits<Frequency_Profiler>::profiled && Boot_Synchronizer::try_acquire())
        Frequency_Profiler::analyse_profiled_data();

    // Transition from CPU-based locking to thread-based locking
    _not_booting = true;
}

__END_SYS
