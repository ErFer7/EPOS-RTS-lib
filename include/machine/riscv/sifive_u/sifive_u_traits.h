// EPOS SiFive-U (RISC-V) Metainfo and Configuration

#ifndef __riscv_sifive_u_traits_h
#define __riscv_sifive_u_traits_h

#include <system/config.h>

__BEGIN_SYS

class Machine_Common;
template<> struct Traits<Machine_Common>: public Traits<Build>
{
protected:
    static const bool library = (Traits<Build>::MODE == Traits<Build>::LIBRARY);
};

template<> struct Traits<Machine>: public Traits<Machine_Common>
{
public:

    // Value to be used for undefined addresses
    static const unsigned long NOT_USED         = -1UL;

    static const bool supervisor = false;

    // CPU numbering
    static const unsigned long CPU_OFFSET       = 1;            // We skip core zero, which is a E CPU without MMU

    // Clocks
    static const unsigned long CLOCK            = 1000000000;                                   // CORECLK
    static const unsigned long HFCLK            =   33330000;                                   // FU540-C000 generates all internal clocks from 33.33 MHz hfclk driven from an external oscillator (HFCLKIN) or crystal (HFOSCIN) input, selected by input HFXSEL.
    static const unsigned long RTCCLK           =    1000000;                                   // The CPU real time clock (rtcclk) runs at 1 MHz and is driven from input pin RTCCLKIN. This should be connected to an external oscillator.
    static const unsigned long TLCLK            = CLOCK / 2;                                    // L2 cache and peripherals such as UART, SPI, I2C, and PWM operate in a single clock domain (tlclk) running at coreclk/2 rate. There is a low-latency 2:1 crossing between coreclk and tlclk domains.

    // Physical Memory
    static const unsigned long RAM_BASE         = 0x80000000;                                   // 2 GB
    static const unsigned long RAM_TOP          = 0x87ffffff;                                   // 2 GB + 128 MB (max 256 GB of RAM + MIO)
    static const unsigned long MIO_BASE         = 0x00000000;
    static const unsigned long MIO_TOP          = 0x1fffffff;                                   // 512 MB

    // Physical Memory at Boot
    static const unsigned long BOOT             = NOT_USED;
    static const unsigned long SETUP            = NOT_USED;
    static const unsigned long IMAGE            = NOT_USED;

    // Logical Memory
    static const unsigned long APP_LOW          = RAM_BASE;
    static const unsigned long APP_HIGH         = RAM_TOP;

    static const unsigned long APP_CODE         = APP_LOW;
    static const unsigned long APP_DATA         = APP_CODE + 4 * 1024 * 1024;                   // APP_CODE + 4 MB

    static const unsigned long PHY_MEM          = NOT_USED;
    static const unsigned long IO               = NOT_USED;
    static const unsigned long SYS              = NOT_USED;

    // Default Sizes and Quantities
    static const unsigned int MAX_THREADS       = 15;
    static const unsigned int STACK_SIZE        = 16 * 1024;
    static const unsigned int HEAP_SIZE         = 4 * 1024 * 1024;
};

template <> struct Traits<IC>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;

    static const unsigned int PLIC_IRQS = 54;           // IRQ0 is used by PLIC to signalize that there is no interrupt being serviced or pending

    struct Interrupt_Source: public _SYS::Interrupt_Source {
        static const unsigned int IRQ_L2_CACHE  = 1;    // 3 contiguous interrupt sources
        static const unsigned int IRQ_UART0     = 4;
        static const unsigned int IRQ_UART1     = 5;
        static const unsigned int IRQ_QSPI2     = 6;
        static const unsigned int IRQ_GPIO0     = 7;    // 16 contiguous interrupt sources
        static const unsigned int IRQ_DMA0      = 23;   // 8 contiguous interrupt sources
        static const unsigned int IRQ_DDR       = 31;
        static const unsigned int IRQ_MSI0      = 32;   // 12 contiguous interrupt sources
        static const unsigned int IRQ_PWM0      = 42;   // 4 contiguous interrupt sources
        static const unsigned int IRQ_PWM1      = 46;   // 4 contiguous interrupt sources
        static const unsigned int IRQ_I2C       = 50;
        static const unsigned int IRQ_QSPI0     = 51;
        static const unsigned int IRQ_QSPI1     = 52;
        static const unsigned int IRQ_ETH0      = 53;
    };
};

template <> struct Traits<Timer>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;

    static const unsigned int UNITS = 1;
    static const unsigned int CLOCK = Traits<Machine>::RTCCLK;

    // Meaningful values for the timer frequency range from 100 to 10000 Hz. The
    // choice must respect the scheduler time-slice, i. e., it must be higher
    // than the scheduler invocation frequency.
    static const int FREQUENCY = 1000; // Hz
};

template <> struct Traits<UART>: public Traits<Machine_Common>
{
    static const unsigned int UNITS = 2;

    static const unsigned int CLOCK = Traits<Machine>::TLCLK;

    static const unsigned int DEF_UNIT = 1;
    static const unsigned int DEF_BAUD_RATE = 115200;
    static const unsigned int DEF_DATA_BITS = 8;
    static const unsigned int DEF_PARITY = 0; // none
    static const unsigned int DEF_STOP_BITS = 1;
};

template <> struct Traits<SPI>: public Traits<Machine_Common>
{
    static const unsigned int UNITS = 3;

    static const unsigned int CLOCK = Traits<Machine>::TLCLK;

    static const unsigned int DEF_UNIT = 0;
    static const unsigned int DEF_PROTOCOL = 0;
    static const unsigned int DEF_MODE = 0;
    static const unsigned int DEF_DATA_BITS = 8;
    static const unsigned int DEF_BIT_RATE = 250000;
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


template<> struct Traits<Ethernet>: public Traits<Machine_Common>
{
    typedef LIST<GEM> DEVICES;
    static const unsigned int UNITS = DEVICES::Length;

    static const bool enabled = (Traits<Build>::NODES > 1) && (UNITS > 0);

    static const bool promiscuous = false;
};

template<> struct Traits<GEM>: public Traits<Ethernet>
{
    static const unsigned int UNITS = DEVICES::Count<GEM>::Result;
    static const bool enabled = Traits<Ethernet>::enabled;

    static const unsigned int SEND_BUFFERS = 64; // per unit
    static const unsigned int RECEIVE_BUFFERS = 64; // per unit
};

__END_SYS

#endif
