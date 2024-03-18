// EPOS ARMv7 MMU Mediator Declarations

#ifndef __armv7_mmu_h
#define __armv7_mmu_h

#define __mmu_common_only__
#include <architecture/mmu.h>
#undef __mmu_common_only__
#include <system/memory_map.h>

__BEGIN_SYS

class ARMv7_MMU
{
public:
    // Section Flags (for single-level, flat memory mapping)
    class Section_Flags
    {
    public:
        // Page Table entry flags
        enum : unsigned long {
            ID   = 0b10 << 0,   // memory section identifier
            B    = 1 << 2,      // bufferable
            C    = 1 << 3,      // cacheable
            XN   = 1 << 4,      // execute never
            AP0  = 1 << 10,
            AP1  = 1 << 11,
            TEX0 = 1 << 12,
            TEX1 = 1 << 13,
            TEX2 = 1 << 14,
            AP2  = 1 << 15,
            S    = 1 << 16,     // shareable
            nG   = 1 << 17,     // non-global (entry in the TLB)
            nS   = 1 << 19,     // non-secure
            FLAT_MEMORY_MEM = (nS | S | AP1 | AP0 |       C | B | ID),
            FLAT_MEMORY_DEV = (nS | S | AP1 | AP0 |       C |     ID),
            FLAT_MEMORY_PER = (nS | S | AP1 | AP0 |  XN |     B | ID)
        };

    public:
        Section_Flags(unsigned long f) : _flags(f) {}
        operator unsigned long() const { return _flags; }

    private:
        unsigned long _flags;
    };
};


class MMU: public No_MMU {};

__END_SYS

#endif
