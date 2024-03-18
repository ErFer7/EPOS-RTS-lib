// EPOS SiFive-U (RISC-V) Metainfo and Configuration

#ifndef __riscv_visionfive2_traits_h
#define __riscv_visionfive2_traits_h

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

    // Clocks
    static const unsigned long CLOCK            =  600000000;                                   // CORECLK
    static const unsigned long HFCLK            =   33330000;                                   // JH7110 generates all internal clocks from 33.33 MHz hfclk driven from an external oscillator (HFCLKIN) or crystal (HFOSCIN) input, selected by input HFXSEL.
    static const unsigned long RTCCLK           =    1000000;                                   // The CPU real time clock (rtcclk) runs at 1 MHz and is driven from input pin RTCCLKIN. This should be connected to an external oscillator.
    static const unsigned long TLCLK            = CLOCK / 2;                                    // L2 cache and peripherals such as UART, SPI, I2C, and PWM operate in a single clock domain (tlclk) running at coreclk/2 rate. There is a low-latency 2:1 crossing between coreclk and tlclk domains.

    // Physical Memory
    static const unsigned long RAM_BASE         = 0x40000000;                                   // 2 GB
    static const unsigned long RAM_TOP          = 0x7fffffff;                                   // 2 GB
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
    static const unsigned long APP_DATA         = APP_CODE + 64 * 1024;

    static const unsigned long PHY_MEM          = RAM_BASE;
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
    static const bool enabled = true;
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

template<> struct Traits<PLIC>: public Traits<Machine_Common>
{
    // Number of external interrupts in SiFive U
    static const unsigned int EIRQS = 127; // TODO: Define how many external interrupts are in SiFive VisionFive2

    // Number of NIC interrupts in SiFive U
    static const unsigned int INT_GIGABIT_ETH = 0; // TODO: Define the ID of the NIC interrupt in SiFive VisionFive2

};

__END_SYS

#endif
