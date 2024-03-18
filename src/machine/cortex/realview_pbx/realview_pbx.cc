// EPOS Realview PBX (ARM Cortex-A9) Mediator Implementation

#include <machine/machine.h>
#include <machine/display.h>

__BEGIN_SYS

void Realview_PBX::reboot()
{
//TODO: reboot!
    CPU::int_disable();
    CPU::halt();
    while(true);
}

__END_SYS
