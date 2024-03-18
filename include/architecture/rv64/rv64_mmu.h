// EPOS RISC-V 64 MMU Mediator Declarations

#ifndef __rv64_mmu_h
#define __rv64_mmu_h

#define __mmu_common_only__
#include <architecture/mmu.h>
#undef __mmu_common_only__
#include <system/memory_map.h>

__BEGIN_SYS

class MMU: public No_MMU {};

__END_SYS

#endif
