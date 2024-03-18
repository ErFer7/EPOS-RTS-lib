// EPOS SiFive-E (RISC-V) Memory Map

#ifndef __riscv_sifive_e_memory_map_h
#define __riscv_sifive_e_memory_map_h

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

        // Memory-mapped devices
        BIOS_BASE       = 0x00001000,   // SiFive-E BIOS ROM
        CLINT_BASE      = 0x02000000,   // SiFive CLINT
        TIMER_BASE      = 0x02004000,   // CLINT Timer
        PLIIC_BASE      = 0x0c000000,   // SiFive PLIC
        AON_BASE        = 0x10000000,   // SiFive-E Always-On (AON) Domain (real-time stuff)
        PRCI_BASE       = 0x10008000,   // SiFive-E Power, Reset, Clock, Interrupt
        GPIO_BASE       = 0x10012000,   // SiFive-E GPIO
        UART0_BASE      = 0x10013000,   // SiFive UART
        SPI0_BASE       = 0x10014000,   // SiFive-E SPI
        PWM0_BASE       = 0x10015000,   // SiFive-E GPIO
        UART1_BASE      = 0x10023000,   // SiFive UART
        SPI1_BASE       = 0x10034000,   // SiFive-E SPI
        PWM1_BASE       = 0x10025000,   // SiFive-E GPIO
        SPI2_BASE       = 0x10034000,   // SiFive-E SPI
        PWM2_BASE       = 0x10035000,   // SiFive-E GPIO
        FLASH_BASE      = 0x20000000,   // SiFive-E XIP

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
