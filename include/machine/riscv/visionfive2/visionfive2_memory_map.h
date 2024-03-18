// EPOS SiFive-U (RISC-V) Memory Map

#ifndef __riscv_visionfive2_memory_map_h
#define __riscv_visionfive2_memory_map_h

#include <system/memory_map.h>

__BEGIN_SYS

struct Memory_Map
{
public:
    enum : unsigned long {
        NOT_USED        = Traits<Machine>::NOT_USED,

        // Physical Memory
        RAM_BASE        = Traits<Machine>::RAM_BASE,
        RAM_TOP         = Traits<Machine>::RAM_TOP,
        MIO_BASE        = Traits<Machine>::MIO_BASE,
        MIO_TOP         = Traits<Machine>::MIO_TOP,
        BOOT_STACK      = RAM_TOP + 1 - Traits<Machine>::STACK_SIZE, // will be used as the stack's base, not the stack pointer
        FREE_BASE       = RAM_BASE,
        FREE_TOP        = BOOT_STACK,

        // Memory-mapped devices           Attribute    Size            Remarks
        CLINT_BASE      = 0x02000000,   //                              CLINT
        TIMER_BASE      = 0x02004000,   //                              CLINT Timer
        PLIC_CPU_BASE   = 0x0c000000,   // SiFive PLIC
        UART0_BASE      = 0x10000000,   // RW A         64KB            Synopsys DW8250
        UART1_BASE      = 0x10010000,   // RW A         64KB
        UART2_BASE      = 0x10020000,   // RW A         64KB
        I2C0_BASE       = 0x10030000,   // RW A         64KB
        I2C1_BASE       = 0x10040000,   // RW A         64KB
        I2C2_BASE       = 0x10050000,   // RW A         64KB
        SPI0_BASE       = 0x10060000,   // RW A         64KB
        SPI1_BASE       = 0x10070000,   // RW A         64KB
        SPI2_BASE       = 0x10080000,   // RW A         64KB
        SPDIF_BASE      = 0x100a0000,   // RW A         64KB
        UART3_BASE      = 0x12000000,   // RW A         64KB
        UART4_BASE      = 0x12010000,   // RW A         64KB
        UART5_BASE      = 0x12020000,   // RW A         64KB
        I2C3_BASE       = 0x12030000,   // RW A         64KB
        I2C4_BASE       = 0x12040000,   // RW A         64KB
        I2C5_BASE       = 0x12050000,   // RW A         64KB
        I2C6_BASE       = 0x12060000,   // RW A         64KB
        SPI3_BASE       = 0x12070000,   // RW A         64KB
        SPI4_BASE       = 0x12080000,   // RW A         64KB
        SPI5_BASE       = 0x12090000,   // RW A         64KB
        SPI6_BASE       = 0x120a0000,   // RW A         64KB
        TIMER1_BASE     = 0x13050000,   // RW A         64KB
        MBOX_BASE       = 0x13060000,   // RW A         64KB
        CAN0_BASE       = 0x130d0000,   // RW A         64KB        2.0B
        SDIO0_BASE      = 0x16010000,   // RW A         64KB
        SDIO1_BASE      = 0x16020000,   // RW A         64KB
        GMAC0_BASE      = 0x16030000,   // RW A         64KB
        GMAC1_BASE      = 0x16040000,   // RW A         64KB
        MAC_DMA_BASE    = 0x16050000,   // RW A         64KB
        RTC_BASE        = 0x17040000,   // RW A         64KB
        OTPC_BASE       = 0x17050000,   // RW A         64KB
        GPU_BASE        = 0x18000000,   // RW A         1MB
        ROM_BASE        = 0x2a000000,   // RX           32KB        Boot ROM

        ETH_BASE = 0x80000000,
        

        // Physical Memory at Boot
        BOOT            = Traits<Machine>::BOOT,
        IMAGE           = Traits<Machine>::IMAGE,
        SETUP           = Traits<Machine>::SETUP,

        // Logical Address Space
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
        INIT            = NOT_USED,
        SYS_HEAP        = NOT_USED,
        SYS_HIGH        = NOT_USED
    };
};

__END_SYS

#endif
