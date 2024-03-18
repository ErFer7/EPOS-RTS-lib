// EPOS ARMv7 Time-Stamp Counter Mediator Implementation

#include <architecture/tsc.h>

__BEGIN_SYS

#ifdef __cortex_m__

volatile TSC::Time_Stamp TSC::_overflow = 0;

#endif

__END_SYS
