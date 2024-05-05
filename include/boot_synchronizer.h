// EPOS Boot Synchronizer Declarations

#ifndef __trylock_h
#define __trylock_h

#include <architecture.h>

__BEGIN_SYS

class Boot_Synchronizer
{
public:
    // When a core tries to acquire a boot task; something that should be done only once, the synchronizer will check
    // if the task was already done and it will return true if the task was not done yet, and false otherwise.
    static bool try_acquire();

private:
    static volatile bool _self_lock;
    static unsigned int _counter[Traits<Build>::CPUS];
    static unsigned int _max;
};

__END_SYS

#endif
