// EPOS Boot Synchronizer Implementations

#include <boot_synchronizer.h>

__BEGIN_SYS

__attribute__((section(".data"))) unsigned int Boot_Synchronizer::_counter[Traits<Build>::CPUS] = {0};
__attribute__((section(".data"))) volatile unsigned int Boot_Synchronizer::_step = 0;
__attribute__((section(".data"))) volatile bool Boot_Synchronizer::_locked = false;

bool Boot_Synchronizer::acquire_single_core_section() {
    db<Boot_Synchronizer>(TRC) << "Boot_Synchronizer::try_acquire()" << endl;

    while(CPU::tsl(_locked));

    if (++_counter[CPU::id()] > _step) {
        _step++;
        CPU::asz(_locked);

        return true;
    }

    CPU::asz(_locked);

    return false;
}

__END_SYS
