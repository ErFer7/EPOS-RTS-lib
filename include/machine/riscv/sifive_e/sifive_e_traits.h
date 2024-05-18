// EPOS SiFive-E (RISC-V) Metainfo and Configuration

#ifndef __riscv_sifive_e_traits_h
#define __riscv_sifive_e_traits_h

#include <system/config.h>

__BEGIN_SYS

class Machine_Common;
template<> struct Traits<Machine_Common>: public Traits<Build>
{
protected:
    static const bool library = (Traits<Build>::SMOD == Traits<Build>::LIBRARY);
};

template<> struct Traits<Machine>: public Traits<Machine_Common>
{
public:
    // Value to be used for undefined addresses
    static const unsigned long NOT_USED         = 0xffffffff;

    // RISC-V mode for library
    static const bool supervisor = false;                                                       // Run EPOS library in supervisor mode

    // CPU numbering
    static const unsigned long CPU_OFFSET       = 0;
    static const unsigned int  BSP              = 0;                                            // Bootstrap/service processor

    // Physical Memory
    static const unsigned long ROM_BASE         = 0x20400000;                           // 516 MB
    static const unsigned long ROM_TOP          = 0x3fffffff;                           // 1 GB
    static const unsigned long RAM_BASE         = 0x80000000;                           // 2 GB
    static const unsigned long RAM_TOP          = 0x80003fff;                           // 2 GB + 16 KB
    static const unsigned long MIO_BASE         = 0x00000000;
    static const unsigned long MIO_TOP          = 0x1fffffff;                           // 512 MB (max 512 MB of MIO => RAM + MIO < 2 G)

    // Physical Memory at Boot
    static const unsigned long BOOT             = NOT_USED;
    static const unsigned long SETUP            = NOT_USED;
    static const unsigned long IMAGE            = NOT_USED;

    // Logical Memory
    static const unsigned long APP_LOW          = RAM_BASE;
    static const unsigned long APP_HIGH         = RAM_TOP;

    static const unsigned long APP_CODE         = ROM_BASE;
    static const unsigned long APP_DATA         = RAM_BASE;

    static const unsigned long PHY_MEM          = NOT_USED;
    static const unsigned long IO               = NOT_USED;
    static const unsigned long SYS              = NOT_USED;

    // Default Sizes and Quantities
    static const unsigned int MAX_THREADS       = 8;
    static const unsigned int STACK_SIZE        = 1024;
    static const unsigned int HEAP_SIZE         = 2048;
};

template <> struct Traits<IC>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;

    static const unsigned int PLIC_IRQS = 53;           // IRQ0 is used by PLIC to signalize that there is no interrupt being serviced or pending

    struct Interrupt_Source: public _SYS::Interrupt_Source {
        static const unsigned int IRQ_GPIO0     = 1;    // 32 contiguous interrupt sources
        static const unsigned int IRQ_UART0     = 33;
        static const unsigned int IRQ_UART1     = 34;
        static const unsigned int IRQ_QSPI0     = 35;
        static const unsigned int IRQ_SPI1      = 36;
        static const unsigned int IRQ_SPI2      = 37;
        static const unsigned int IRQ_PWM0      = 38;   // 4 contiguous interrupt sources
        static const unsigned int IRQ_PWM1      = 42;   // 4 contiguous interrupt sources
        static const unsigned int IRQ_PWM2      = 46;   // 4 contiguous interrupt sources
        static const unsigned int IRQ_I2C       = 50;
        static const unsigned int IRQ_WDOG      = 51;
        static const unsigned int IRQ_RTC       = 52;
    };
};

template <> struct Traits<Timer>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;

    static const unsigned int UNITS = 1;
    static const unsigned int CLOCK = 10000000;

    // Meaningful values for the timer frequency range from 100 to 10000 Hz. The
    // choice must respect the scheduler time-slice, i. e., it must be higher
    // than the scheduler invocation frequency.
    static const int FREQUENCY = 100; // Hz
};

template <> struct Traits<UART>: public Traits<Machine_Common>
{
    static const unsigned int UNITS = 2;

    static const unsigned int CLOCK = 22729000;

    static const unsigned int DEF_UNIT = 1;
    static const unsigned int DEF_BAUD_RATE = 115200;
    static const unsigned int DEF_DATA_BITS = 8;
    static const unsigned int DEF_PARITY = 0; // none
    static const unsigned int DEF_STOP_BITS = 1;
};

template<> struct Traits<Serial_Display>: public Traits<Machine_Common>
{
    static const bool enabled = (Traits<Build>::EXPECTED_SIMULATION_TIME != 0);
    static const int ENGINE = UART;
    static const int UNIT = 1;
    static const int COLUMNS = 80;
    static const int LINES = 24;
    static const int TAB_SIZE = 8;
};

template<> struct Traits<Scratchpad>: public Traits<Machine_Common>
{
    static const bool enabled = false;
};

__END_SYS

#endif
