// EPOS Realview PBX (Cortex-A9) Metainfo and Configuration

#ifndef __realview_pbx_traits_h
#define __realview_pbx_traits_h

#include <system/config.h>

__BEGIN_SYS

class Machine_Common;
template<> struct Traits<Machine_Common>: public Traits<Build>
{
    typedef IF<SMOD != LIBRARY, void, bool>::Result Sanity;
    static const Sanity sane = true;

    static const bool debugged = Traits<Build>::debugged;
};

template<> struct Traits<Machine>: public Traits<Machine_Common>
{
    static const unsigned int NOT_USED          = 0xffffffff;

    // Physical Memory
    static const unsigned int RAM_BASE          = 0x00000000;
    static const unsigned int RAM_TOP           = 0x07ffffff;   // 128 MB
    static const unsigned int MIO_BASE          = 0x10000000;
    static const unsigned int MIO_TOP           = 0x1fffffff;

    // Physical Memory at Boot
    static const unsigned int BOOT              = NOT_USED;
    static const unsigned int IMAGE             = NOT_USED;
    static const unsigned int RESET             = 0x00010000; // QEMU requires the vector table (i.e. SETUP's entry point) to be here
    static const unsigned int SETUP             = NOT_USED;

    // Logical Memory Map (this machine only supports mode LIBRARY, so these are indeed also physical addresses)
    static const unsigned int APP_LOW           = RESET;
    static const unsigned int APP_HIGH          = RAM_TOP;
    static const unsigned int APP_CODE          = APP_LOW;
    static const unsigned int APP_DATA          = APP_LOW;

    static const unsigned int INIT              = NOT_USED;
    static const unsigned int PHY_MEM           = NOT_USED;
    static const unsigned int IO                = NOT_USED;
    static const unsigned int SYS               = NOT_USED;

    // Default Sizes and Quantities
    static const unsigned int MAX_THREADS       = 16;
    static const unsigned int STACK_SIZE        = 16 * 1024;
    static const unsigned int HEAP_SIZE         = 4 * 1024 * 1024;

    // PLL clocks
    static const unsigned int ARM_PLL_CLOCK     = 1333333333;
    static const unsigned int IO_PLL_CLOCK      = 1000000000;
    static const unsigned int DDR_PLL_CLOCK     = 1066666666;
};

template<> struct Traits<IC>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;
};

template<> struct Traits<Timer>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;

    static const unsigned int UNITS = 1; // ARM Cortex-A9 Global Timer

    // Meaningful values for the timer frequency range from 100 to 10000 Hz. The
    // choice must respect the scheduler time-slice, i. e., it must be higher
    // than the scheduler invocation frequency.
    static const int FREQUENCY = 1000; // Hz
};

template<> struct Traits<UART>: public Traits<Machine_Common>
{
    static const unsigned int UNITS = 2;

    // CLOCK_DIVISOR is hard coded in ps7_init.tcl
    static const unsigned int CLOCK_DIVISOR = 20;
    static const unsigned int CLOCK = Traits<Machine>::IO_PLL_CLOCK/CLOCK_DIVISOR;

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

template<> struct Traits<Serial_Keyboard>: public Traits<Machine_Common>
{
    static const bool enabled = (Traits<Build>::EXPECTED_SIMULATION_TIME != 0);
};

template<> struct Traits<Scratchpad>: public Traits<Machine_Common>
{
    static const bool enabled = false;
};

template<> struct Traits<Ethernet>: public Traits<Machine_Common>
{
    // NICS that don't have a network in Traits<Network>::NETWORKS will not be enabled
    typedef LIST<Ethernet_NIC> DEVICES;
    static const unsigned int UNITS = DEVICES::Length;

    static const bool enabled = (Traits<Build>::NETWORKING != STANDALONE) && (UNITS > 0);
    static const bool promiscuous = false;
};

__END_SYS

#endif
