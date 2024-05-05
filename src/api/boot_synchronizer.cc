// EPOS Boot Synchronizer Implementations

#include <boot_synchronizer.h>

__BEGIN_SYS

volatile bool Boot_Synchronizer::_self_lock = false;
unsigned int Boot_Synchronizer::_counter[Traits<Build>::CPUS] = {0};
unsigned int Boot_Synchronizer::_max = 0;

bool Boot_Synchronizer::try_acquire() {
    db<Boot_Synchronizer>(TRC) << "Boot_Synchronizer::try_acquire()" << endl;

    while(CPU::tsl(_self_lock));

    if (++_counter[CPU::id()] > _max) {
        _max++;
        _self_lock = false;

        return true;
    }

    _self_lock = false;

    return false;
}

__END_SYS
