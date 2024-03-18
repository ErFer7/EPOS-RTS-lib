// EPOS ARMv8 MMU Mediator Declarations

#ifndef __armv8_mmu_h
#define __armv8_mmu_h

#define __mmu_common_only__
#include <architecture/mmu.h>
#undef __mmu_common_only__
#include <system/memory_map.h>

__BEGIN_SYS

class ARMv8_MMU
{
public:
    // Page Flags
    class Page_Flags
    {
    public:
        // TTBR bits
        enum : unsigned long {
            INVALID             = 0UL   << 0,
            RESERVED            = 0b01  << 0,
            PAGE_DESCRIPTOR     = 0b11  << 0,
            BLOCK_DESCRIPTOR    = 0b01  << 0,
            SEL_MAIR_ATTR0      = 0b00  << 2,
            SEL_MAIR_ATTR1      = 0b01  << 2,
            SEL_MAIR_ATTR2      = 0b10  << 2,
            SEL_MAIR_ATTR3      = 0b11  << 2,
            SEL_MAIR_ATTR4      = 0b100 << 2,
            SEL_MAIR_ATTR5      = 0b101 << 2,
            SEL_MAIR_ATTR6      = 0b110 << 2,
            SEL_MAIR_ATTR7      = 0b111 << 2,
            NON_SHAREABLE       = 0UL   << 8,
            UNPREDICTABLE       = 0b01  << 8,
            OUTER_SHAREABLE     = 0b10  << 8,
            INNER_SHAREABLE     = 0b11  << 8,
            NS_LEVEL1           = 0b1   << 5, // Output address is in secure address
            AP1                 = 0b1   << 6,
            AP2                 = 0b1   << 7,
            RW_SYS              = 0b0   << 6,
            RW_USR              = AP1,
            RO_SYS              = AP2,
            RO_USR              = (AP2 | AP1),
            ACCESS              = 0b1   << 10,
            nG                  = 1     << 11,
            CONTIGUOUS          = 0b1ULL<< 52, // Output memory is contiguous
            EL1_XN              = 0b1ULL<< 53, // el1 cannot execute
            XN                  = 0b1ULL<< 54, // el0 cannot execute
            DEV                 = SEL_MAIR_ATTR0,
            CWT                 = SEL_MAIR_ATTR1,
            CWB                 = SEL_MAIR_ATTR2,
            CD                  = SEL_MAIR_ATTR3,

            FLAT_MEM_PT         = (PAGE_DESCRIPTOR  | INNER_SHAREABLE | SEL_MAIR_ATTR0 | ACCESS),
            FLAT_MEM_BLOCK      = (BLOCK_DESCRIPTOR | INNER_SHAREABLE | SEL_MAIR_ATTR0 | ACCESS),

            PTE                 = (PAGE_DESCRIPTOR | ACCESS),

            APP                 = (nG | INNER_SHAREABLE | RW_SYS | CWT | PTE),          // S, RWX  All, Normal WT
            APPD                = (nG | INNER_SHAREABLE | RW_SYS | CWT | XN  | PTE),    // S, RWnX All, Normal WT
            APPC                = (nG | INNER_SHAREABLE | RW_SYS | CWT | PTE),          // S, RnWX All, Normal WT
            SYSD                = (nG | INNER_SHAREABLE | RW_SYS | PTE),                // S, RWX  SYS, Normal WT
            SYSC                = (nG | INNER_SHAREABLE | RW_SYS | PTE),                // S, RWX  SYS, Normal WT
            IO                  = (nG | RW_USR | DEV | PTE),                            // Device Memory = Shareable, RWX, SYS
            DMA                 = (nG | RO_SYS | DEV | PTE),                            // Device Memory no cacheable / Old Peripheral = Shareable, RWX, B ?
        };

    public:
        Page_Flags() {}
        Page_Flags(unsigned long f) : _flags(f) {}

        operator unsigned long() const { return _flags; }

        friend OStream & operator<<(OStream & os, const Page_Flags & f) { os << hex << f._flags; return os; }

    private:
        unsigned int _flags;
    };
};


class MMU: public No_MMU {};

__END_SYS

#endif
