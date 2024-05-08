// EPOS Boot Synchronizer Implementations

#include <boot_synchronizer.h>

__BEGIN_SYS

unsigned int Boot_Synchronizer::_counter[Traits<Build>::CPUS] = {0, 0, 0, 0};
unsigned int Boot_Synchronizer::_max = 0;
volatile bool Boot_Synchronizer::_locked = false;

bool Boot_Synchronizer::try_acquire() {
    db<Boot_Synchronizer>(TRC) << "Boot_Synchronizer::try_acquire()" << endl;

    while(CPU::tsl(_locked));

    if (++_counter[CPU::id()] > _max) {
        CPU::finc(_max);
        CPU::asz(_locked);

        return true;
    }

    CPU::asz(_locked);

    return false;
}

__END_SYS
