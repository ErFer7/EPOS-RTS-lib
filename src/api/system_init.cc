// EPOS System Initialization

#include <boot_synchronizer.h>
#include <system.h>
#include <time.h>
#include <process.h>

__BEGIN_SYS

void System::init()
{
    if(Traits<Alarm>::enabled && Boot_Synchronizer::try_acquire())
        Alarm::init();

    if(Traits<Thread>::enabled)
        Thread::init();
}

__END_SYS
