// EPOS IA32 CPU Mediator Initialization

#include <architecture.h>
#include <machine/machine.h>
#include <system.h>

__BEGIN_SYS

void CPU::init()
{
    db<Init, CPU>(TRC) << "CPU::init()" << endl;

    _cpu_clock = System::info()->tm.cpu_clock;
    _cpu_current_clock = System::info()->tm.cpu_clock;
    _bus_clock = System::info()->tm.bus_clock;

    // Initialize the MMU
    if(Traits<MMU>::enabled)
        MMU::init();
    else
        db<Init, MMU>(WRN) << "MMU is disabled!" << endl;

    // Initialize the PMU	
    if(Traits<PMU>::enabled)
        PMU::init();
}

__END_SYS
