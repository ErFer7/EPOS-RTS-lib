// EPOS Boot Synchronizer Declarations

#ifndef __boot_synchronizer_h
#define __boot_synchronizer_h

#include <architecture.h>

__BEGIN_SYS

class Boot_Synchronizer
{
public:
    // When a core tries to acquire a boot task; something that should be done only once, the synchronizer will check
    // if the task was already done and it will return true if the task was not done yet, and false otherwise.
    static bool acquire_single_core_section();

private:
    static unsigned int _counter[Traits<Build>::CPUS];
    static volatile unsigned int _step;
    static volatile bool _locked;
};

__END_SYS

#endif
