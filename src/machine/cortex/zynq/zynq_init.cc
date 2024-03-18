// EPOS Zynq (ARM Cortex-A9) Initialization

#include <machine.h>

__BEGIN_SYS

void Zynq::init()
{
    unlock_slcr();
    fpga_reset();
}

__END_SYS
