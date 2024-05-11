// EPOS CPU Mediator Implementation

#include <architecture/cpu.h>

__BEGIN_SYS

__attribute__((section(".data"))) volatile int CPU_Common::_ready[2] = {0, 0};
__attribute__((section(".data"))) volatile int CPU_Common::_i = 0;

__END_SYS
