// EPOS ARMv8 PMU Mediator Initialization

#include <architecture/pmu.h>

__BEGIN_SYS

void ARMv8_A_PMU::init()
{
    db<Init, PMU>(TRC) << "PMU::init()" << endl;

    // Set global enable, reset all counters including cycle counter, and
    // export the events
    // Enable the PMU acess on EL0
    pmuserenr_el0(pmuserenr_el0() | PMUSERENR_EN_EL0);


    // Set global enable and reset all counters including cycle counter
    pmcr_el0(pmcr_el0() | PMCR_E | PMCR_P | PMCR_C);


    // Enable cycle counter
    pmcntenset_el0(pmcntenset_el0() | PMCNTENSET_EN_EL0);
}

__END_SYS