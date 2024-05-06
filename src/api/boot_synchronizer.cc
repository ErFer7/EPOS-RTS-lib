// EPOS Boot Synchronizer Implementations

#include <boot_synchronizer.h>

__BEGIN_SYS

volatile unsigned int Boot_Synchronizer::_counter[Traits<Build>::CPUS] = {0};
volatile unsigned int Boot_Synchronizer::_max = 0;

bool Boot_Synchronizer::try_acquire() {
    db<Boot_Synchronizer>(TRC) << "Boot_Synchronizer::try_acquire()" << endl;

    if (++_counter[CPU::id()] > _max) {
        CPU::finc(_max);

        return true;
    }

    return false;
}

__END_SYS
