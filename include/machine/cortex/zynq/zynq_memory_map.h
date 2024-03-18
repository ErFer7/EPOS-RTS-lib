// EPOS Zynq-7000 (Cortex-A9) Memory Map

#ifndef __zynq_memory_map_h
#define __zynq_memory_map_h

#include <machine/cortex/cortex_memory_map.h>

__BEGIN_SYS

struct Memory_Map: public Cortex_Memory_Map
{
    enum {
        // Base addresses for memory-mapped control and I/O devices
        UART0_BASE              = 0xe0000000,
        UART1_BASE              = 0xe0001000,
        SPI0_BASE               = 0xe0006000,
        SPI1_BASE               = 0xe0007000,
        ETH0_BASE               = 0xe000b000,
        ETH1_BASE               = 0xe000c000,
        SPI2_BASE               = 0xe000d000,
        SD0_BASE                = 0xe0100000,
        SD1_BASE                = 0xe0101000,
        FLASH_BASE              = 0xe2000000,
        SLCR_BASE               = 0xf8000000,
        TIMER0_BASE             = 0xf8001000,
        TIMER1_BASE             = 0xf8002000,
        DMA_BASE                = 0xf8003000,
        ADC_BASE                = 0xf8007100,
        PPS_BASE                = 0xf8f00000, // A9 Private Peripheral Space
        SCU_BASE                = 0xf8f00000, // A9 MP Snoop Control Unit
        GIC_CPU_BASE            = 0xf8f00100,
        GLOBAL_TIMER_BASE       = 0xf8f00200,
        TSC_BASE                = GLOBAL_TIMER_BASE,
        PRIVATE_TIMER_BASE      = 0xf8f00600,
        PRIVATE_TIMER_BASE1     = 0xf8f00620,
        GIC_DIST_BASE           = 0xf8f01000,
        
        VECTOR_TABLE            = RAM_BASE, // 8 x 4b instructions + 8 x 4b pointers
        FLAT_PAGE_TABLE         = (RAM_TOP - 16 * 1024) & ~(0x3fff),  // used only with No_MMU in LIBRARY mode; 16KB, 4096 4B entries, each pointing to 1 MB regions, thus mapping up to 4 GB; 16K-aligned for TTBR;
        BOOT_STACK              = FLAT_PAGE_TABLE - Traits<Machine>::STACK_SIZE, // will be used as the stack's base, not the stack pointer
        FREE_TOP                = BOOT_STACK,

        // Logical Address Space
        APP_LOW                 = Traits<Machine>::APP_LOW,
        APP_HIGH                = Traits<Machine>::APP_HIGH,
        APP_CODE                = Traits<Machine>::APP_CODE,
        APP_DATA                = Traits<Machine>::APP_DATA,

        SYS_CODE                = NOT_USED,
        SYS_INFO                = NOT_USED,
        SYS_DATA                = NOT_USED,
        SYS_STACK               = NOT_USED,
        SYS_HEAP                = NOT_USED
    };
};

__END_SYS

#endif
