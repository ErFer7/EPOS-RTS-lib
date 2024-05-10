// EPOS Boot Synchronizer Implementations

#include <boot_synchronizer.h>

__BEGIN_SYS

// This is safe because the Boot_Synchronizer is used only in the boot sequence.
__attribute__((section(".data"))) unsigned int Boot_Synchronizer::_counter[Traits<Build>::CPUS] = {0};
__attribute__((section(".data"))) volatile unsigned int Boot_Synchronizer::_max = 0;
__attribute__((section(".data"))) volatile bool Boot_Synchronizer::_locked = false;

// TODO: Optimize this
bool Boot_Synchronizer::try_acquire() {
    db<Boot_Synchronizer>(TRC) << "Boot_Synchronizer::try_acquire()" << endl;

    while(CPU::tsl(_locked));

    if (++_counter[CPU::id()] > _max) {
        _max++;
        ASM("fence.i" ::: "memory");
        CPU::asz(_locked);

        return true;
    }

    CPU::asz(_locked);

    return false;
}

__END_SYS
