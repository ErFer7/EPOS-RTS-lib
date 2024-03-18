// EPOS Raspberry Pi3 (ARM Cortex-A53) Memory Map

#ifndef __raspberry_pi3_memory_map_h
#define __raspberry_pi3_memory_map_h

#include <machine/cortex/cortex_memory_map.h>

__BEGIN_SYS

struct Memory_Map: public Cortex_Memory_Map
{
private:
    static const bool armv7     = (Traits<Build>::ARCHITECTURE == Traits<Build>::ARMv7);
    static const bool emulated  = Traits<Build>::EXPECTED_SIMULATION_TIME;

public:
    enum : unsigned long {
        // Base addresses for memory-mapped control and I/O devices
        MBOX_COM_BASE   = 0x3ef00000,             // RAM memory for device-os communication (must be mapped as device by the MMU)
        MBOX_CTRL_BASE  = 0x40000000,             // BCM MailBox
        PPS_BASE        = 0x3f000000,             // Private Peripheral Space
        TSC_BASE        = 0x3f003000,
        TIMER0_BASE     = emulated ? MBOX_CTRL_BASE : 0x3f003000,             // System Timer (free running)
        DMA0_BASE       = 0x3f007000,
        IC_BASE         = 0x3f00b200,
        TIMER1_BASE     = 0x3f00b400,             // ARM Timer (frequency relative to processor frequency)
        MBOX_BASE       = 0x3f00b800,
        ARM_MBOX0       = 0x3f00b880,             // IOCtrl (MBOX 0) is also mapped on this address
        PM_BASE         = 0x3f100000,             // Power Manager
        RAND_BASE       = 0x3f104000,
        GPIO_BASE       = 0x3f200000,
        UART_BASE       = 0x3f201000,             // PrimeCell PL011 UART
        SD0_BASE        = 0x3f202000,             // Custom sdhci controller
        AUX_BASE        = 0x3f215000,             // mini UART + 2 x SPI master
        SD1_BASE        = 0x3f300000,             // Arasan sdhci controller
        DMA1_BASE       = 0x3fe05000,

        VECTOR_TABLE    = RAM_BASE,
        FLAT_PAGE_TABLE = (RAM_TOP - 16 * 1024) & ~(0x3fff), // used only with No_MMU in LIBRARY mode; 32-bit: 16KB, 4096 4B entries, each pointing to 1 MB regions, thus mapping up to 4 GB; 64-bit: 16KB, 2048 8B entries, each pointing to 32 MB regions, thus mapping up to 64 GB; 16K-aligned for TTBR;
        BOOT_STACK      = FLAT_PAGE_TABLE - Traits<Machine>::STACK_SIZE, // will be used as the stack's base, not the stack pointer

        FREE_BASE       = VECTOR_TABLE + (armv7 ? 4 : 16) * 1024,
        FREE_TOP        = BOOT_STACK,

        // Logical Address Space -- Need to be verified
        APP_LOW         = Traits<Machine>::APP_LOW,
        APP_HIGH        = Traits<Machine>::APP_HIGH,

        APP_CODE        = Traits<Machine>::APP_CODE,
        APP_DATA        = Traits<Machine>::APP_DATA,

        PHY_MEM         = Traits<Machine>::PHY_MEM,
        IO              = Traits<Machine>::IO,
        SYS             = Traits<Machine>::SYS,
        SYS_CODE        = NOT_USED,
        SYS_INFO        = NOT_USED,
        SYS_PT          = NOT_USED,
        SYS_PD          = NOT_USED,
        SYS_DATA        = NOT_USED,
        SYS_STACK       = NOT_USED,
        SYS_HEAP        = NOT_USED,
        SYS_HIGH        = NOT_USED
    };
};

__END_SYS

#endif
