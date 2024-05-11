// EPOS RISC-V 64 CPU Mediator Initialization

#include <architecture.h>
#include <boot_synchronizer.h>

extern "C" { void __epos_library_app_entry(void); }

__BEGIN_SYS

void CPU::init()
{
    db<Init, CPU>(TRC) << "CPU::init()" << endl;

    if(Boot_Synchronizer::try_acquire()) {
        if(Traits<MMU>::enabled)
            MMU::init();
        else
            db<Init, MMU>(WRN) << "MMU is disabled!" << endl;
    }

    CPU::smp_barrier();

#ifdef __TSC_H
    if(Traits<TSC>::enabled)
        TSC::init();
#endif

    // TODO: Should we put a barrier here?

#ifdef __PMU_H
    if(Traits<PMU>::enabled)
        PMU::init();
#endif
}

__END_SYS
