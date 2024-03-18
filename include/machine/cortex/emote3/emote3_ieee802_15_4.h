// EPOS EPOSMoteIII (ARM Cortex-M3) 802.15.4 NIC Mediator Declarations

#ifndef __emote3_ieee802_15_4_h
#define __emote3_ieee802_15_4_h

#include <system/config.h>

#ifdef __NIC_H

#include <utility/convert.h>
#include <machine/machine.h>
#include <machine/ic.h>
#include <machine/gpio.h>
#include <network/ieee802_15_4.h>
#include "emote3_sysctrl.h"
#include "emote3_memory_map.h"

__BEGIN_SYS

class IEEE802_15_4_Engine;

// TI CC2538 IEEE 802.15.4 RF Transceiver
class CC2538RF
{
    // This is a hardware object.
    // Use with something like "new (Memory_Map::RF_BASE) CC2538RF"
    
    friend IEEE802_15_4_Engine;

protected:
    struct Reset_Backdoor_Message {
        Reset_Backdoor_Message(unsigned char * id) : _header_code(0xbe), _footer_code(0xef), _crc(0) {
            memcpy(_id, id, 8);
        }

    private:
        unsigned char _header_code;
        unsigned char _id[8];
        unsigned char _footer_code;
        unsigned short _crc;
    }__attribute__((packed));

    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg16 Reg16;
    typedef CPU::Reg32 Reg32;
    typedef CPU::Reg64 Reg64;

    static const bool promiscuous = Traits<IEEE802_15_4_NIC>::promiscuous || Traits<TSTP>::enabled;

    static const unsigned int MIN_CHANNEL = 10;
    static const unsigned int MAX_CHANNEL = 27;

    static const unsigned int TX_TO_RX_DELAY = 2; // Radio takes extra 2us to go from TX to RX or idle
    static const unsigned int RX_TO_TX_DELAY = 0;

    static const unsigned int SLEEP_TO_TX_DELAY = 194;
    static const unsigned int SLEEP_TO_RX_DELAY = 194;
    static const unsigned int TX_TO_SLEEP_DELAY = 50; // TODO

public:
    // Registers offsets from BASE (i.e. this)
    enum {
        RXFIFO   = 0x000,
        TXFIFO   = 0x200,
        FFSM     = 0x500,
        XREG     = 0x600,
        SFR      = 0x800,
        MACTIMER = 0x800,
        ANA      = 0x800
    };

    // Useful FFSM register offsets
    enum {
        SRCRESINDEX = 0x8c,
        PAN_ID0     = 0xc8,
        PAN_ID1     = 0xcc,
        SHORT_ADDR0 = 0xd0,
        SHORT_ADDR1 = 0xd4,
    };

    // XREG register offsets
    enum {
        FRMFILT0      = 0x000,
        FRMFILT1      = 0x004,
        SRCMATCH      = 0x008,
        FRMCTRL0      = 0x024,
        FRMCTRL1      = 0x028,
        RXENABLE      = 0x02c,
        RXMASKSET     = 0x030,
        RXMASKCLR     = 0x034,
        FREQCTRL      = 0x03c,   // RF carrier frequency (f = 11 + 5 (k – 11), with k [11, 26])  ro      0x0000000b
        FSMSTAT1      = 0x04c,   // Radio status register                                        ro      0x00000000
        FIFOPCTRL     = 0x050,
        FSMCTRL       = 0x054,
        RXFIRST       = 0x068,
        RXFIFOCNT     = 0x06c,   // Number of bytes in RX FIFO                                   ro      0x00000000
        TXFIFOCNT     = 0x070,   // Number of bytes in TX FIFO                                   ro      0x00000000
        RXFIRST_PTR   = 0x074,
        RXLAST_PTR    = 0x078,
        RXP1_PTR      = 0x07c,
        RFIRQM0       = 0x08c,
        RFIRQM1       = 0x090,
        RFERRM        = 0x094,
        CSPT          = 0x194,
        AGCCTRL1      = 0x0c8,
        TXFILTCFG     = 0x1e8,
        FSCAL1        = 0x0b8,
        CCACTRL0      = 0x058,
        TXPOWER       = 0x040,
        RSSI          = 0x060,
        RSSISTAT      = 0x064,   // RSSI valid status register                                   ro      0x00000000
        RFC_OBS_CTRL0 = 0x1ac, // Select which signal is represented by OBSSEL_SIG0
        RFC_OBS_CTRL1 = 0x1b0, // Select which signal is represented by OBSSEL_SIG1
        RFC_OBS_CTRL2 = 0x1b4, // Select which signal is represented by OBSSEL_SIG2
    };

    // CCTEST register offsets
    enum {                     // Description                                           Type Size Reset
        CCTEST_IO      = 0x00, // Output strength control                               RW   32   0x0000 0000
        CCTEST_OBSSEL0 = 0x14, // Select output signal on GPIO C 0                      RW   32   0x0000 0000
        CCTEST_OBSSEL1 = 0x18, // Select output signal on GPIO C 1                      RW   32   0x0000 0000
        CCTEST_OBSSEL2 = 0x1c, // Select output signal on GPIO C 2                      RW   32   0x0000 0000
        CCTEST_OBSSEL3 = 0x20, // Select output signal on GPIO C 3                      RW   32   0x0000 0000
        CCTEST_OBSSEL4 = 0x24, // Select output signal on GPIO C 4                      RW   32   0x0000 0000
        CCTEST_OBSSEL5 = 0x28, // Select output signal on GPIO C 5                      RW   32   0x0000 0000
        CCTEST_OBSSEL6 = 0x2c, // Select output signal on GPIO C 6                      RW   32   0x0000 0000
        CCTEST_OBSSEL7 = 0x30, // Select output signal on GPIO C 7                      RW   32   0x0000 0000
        CCTEST_TR0     = 0x34, // Used to connect the temperature sensor to the SOC_ADC RW   32   0x0000 0000
        CCTEST_USBCTRL = 0x50, // USB PHY stand-by control                              RW   32   0x0000 0000
    };

    enum OBSSEL {
        OBSSEL_EN      = 1 << 7, // Signal X enable bit
        OBSSEL_SIG0    = 0,      // Select signal 0
        OBSSEL_SIG1    = 1,      // Select signal 1
        OBSSEL_SIG2    = 2,      // Select signal 2
        OBSSEL_SIG0_EN = OBSSEL_EN | OBSSEL_SIG0, // Select and enable signal 0
        OBSSEL_SIG1_EN = OBSSEL_EN | OBSSEL_SIG1, // Select and enable signal 1
        OBSSEL_SIG2_EN = OBSSEL_EN | OBSSEL_SIG2, // Select and enable signal 2
    };

    // Controls which observable signal from RF Core is to be muxed out to OBSSEL_SIGx
    enum SIGNAL {                       // Description
        SIGNAL_LOW              = 0x00, // 0 Constant value
        SIGNAL_HIGH             = 0x01, // 1 Constant value
        SIGNAL_RFC_SNIFF_DATA   = 0x08, // Data from packet sniffer. Sample data on rising edges of sniff_clk.
        SIGNAL_RFC_SNIFF_CLK    = 0x09, // 250kHz clock for packet sniffer data.
        SIGNAL_RSSI_VALID       = 0x0c, // Pin is high when the RSSI value has been
                                        // updated at least once since RX was started. Cleared when leaving RX.
        SIGNAL_DEMOD_CCA        = 0x0d, // Clear channel assessment. See FSMSTAT1 register for details
                                        // on how to configure the behavior of this signal.
        SIGNAL_SAMPLED_CCA      = 0x0e, // A sampled version of the CCA bit from demodulator. The value is
                                        // updated whenever a SSAMPLECCA or STXONCCA strobe is issued.
        SIGNAL_SFD_SYNC         = 0x0f, // Pin is high when a SFD has been received or transmitted.
                                        // Cleared when leaving RX/TX respectively.
                                        // Not to be confused with the SFD exception.
        SIGNAL_TX_ACTIVE        = 0x10, // Indicates that FFCTRL is in one of the TX states. Active-high.
                                        // Note: This signal might have glitches, because it has no output flip-
                                        // flop and is based
                                        // on the current state register of the FFCTRL FSM.
        SIGNAL_RX_ACTIVE        = 0x11, // Indicates that FFCTRL is in one of the RX states. Active-high.
                                        // Note: This signal might have glitches, because it has no output flip-
                                        // flop and is based
                                        // on the current state register of the FFCTRL FSM.
        SIGNAL_FFCTRL_FIFO      = 0x12, // Pin is high when one or more bytes are in the RXFIFO.
                                        // Low during RXFIFO overflow.
        SIGNAL_FFCTRL_FIFOP     = 0x13, // Pin is high when the number of bytes in the RXFIFO exceeds the
                                        // programmable threshold or at least one complete frame is in the RXFIFO.
                                        // Also high during RXFIFO overflow.
                                        // Not to be confused with the FIFOP exception.
        SIGNAL_PACKET_DONE      = 0x14, // A complete frame has been received. I.e.,
                                        // the number of bytes set by the frame-length field has been received.
        SIGNAL_RFC_XOR_RAND_I_Q = 0x16, // XOR between I and Q random outputs. Updated at 8 MHz.
        SIGNAL_RFC_RAND_Q       = 0x17, // Random data output from the Q channel of the receiver. Updated at 8 MHz.
        SIGNAL_RFC_RAND_I       = 0x18, // Random data output from the I channel of the receiver. Updated at 8 MHz.
        SIGNAL_LOCK_STATUS      = 0x19, // 1 when PLL is in lock, otherwise 0
        SIGNAL_PA_PD            = 0x28, // Power amplifier power-down signal
        SIGNAL_LNA_PD           = 0x2a, // LNA power-down signal
    };

    // SFR register offsets
    enum {
        RFDATA  = 0x28,
        RFERRF  = 0x2c,
        RFIRQF1 = 0x30,
        RFIRQF0 = 0x34,
        RFST    = 0x38,         // RF CSMA-CA/strobe processor                                  rw       0x000000d0
    };

    // MACTIMER register offsets
    enum {       //Offset   Description                               Type    Value after reset
        MTCSPCFG = 0x00, // MAC Timer event configuration              RW     0x0
        MTCTRL   = 0x04, // MAC Timer control register                 RW     0x2
        MTIRQM   = 0x08, // MAC Timer interrupt mask                   RW     0x0
        MTIRQF   = 0x0c, // MAC Timer interrupt flags                  RW     0x0
        MTMSEL   = 0x10, // MAC Timer multiplex select                 RW     0x0
        MTM0     = 0x14, // MAC Timer multiplexed register 0           RW     0x0
        MTM1     = 0x18, // MAC Timer multiplexed register 1           RW     0x0
        MTMOVF2  = 0x1c, // MAC Timer multiplexed overflow register 2  RW     0x0
        MTMOVF1  = 0x20, // MAC Timer multiplexed overflow register 1  RW     0x0
        MTMOVF0  = 0x24, // MAC Timer multiplexed overflow register 0  RW     0x0
    };

    // ANA_REGS register offsets
    enum {
        IVCTRL    = 0x04,
    };

    // Radio commands
    enum {
        SRXON       = 0xd3,
        STXON       = 0xd9,
        SFLUSHRX    = 0xdd,
        SFLUSHTX    = 0xde,
        SRFOFF      = 0xdf,
        ISSTART     = 0xe1,
        ISSTOP      = 0xe2,
        ISRXON      = 0xe3,
        ISTXON      = 0xe9,
        ISTXONCCA   = 0xea,
        ISSAMPLECCA = 0xeb,
        ISFLUSHRX   = 0xed,
        ISFLUSHTX   = 0xee,
        ISRFOFF     = 0xef,
        ISCLEAR     = 0xff,
    };

    // Useful bits in RXENABLE
    enum {
        RXENABLE_SRXON         = 1 << 7,
        RXENABLE_STXON         = 1 << 6,
        RXENABLE_SRXMASKBITSET = 1 << 5,
    };

    // Useful bits in RSSISTAT
    enum {
        RSSI_VALID = 1 << 0,
    };

    // Useful bits in XREG_FRMFILT0
    enum {
        MAX_FRAME_VERSION = 1 << 2,
        PAN_COORDINATOR   = 1 << 1,
        FRAME_FILTER_EN   = 1 << 0,
    };

    // Useful bits in XREG_FRMFILT1
    enum {
        ACCEPT_FT3_MAC_CMD = 1 << 6,
        ACCEPT_FT2_ACK     = 1 << 5,
        ACCEPT_FT1_DATA    = 1 << 4,
        ACCEPT_FT0_BEACON  = 1 << 3,
    };

    // Useful bits in XREG_SRCMATCH
    enum {
        SRC_MATCH_EN   = 1 << 0,
    };

    // Useful bits in XREG_FSMCTRL
    enum {
        SLOTTED_ACK    = 1 << 1,
        RX2RX_TIME_OFF = 1 << 0,
    };

    // Useful bits in XREG_FRMCTRL0
    enum {
        APPEND_DATA_MODE = 1 << 7,
        AUTO_CRC         = 1 << 6,
        AUTO_ACK         = 1 << 5,
        ENERGY_SCAN      = 1 << 4,
        RX_MODE          = 1 << 2,
        TX_MODE          = 1 << 0,
    };

    enum RX_MODES {
        RX_MODE_NORMAL = 0,
        RX_MODE_OUTPUT_TO_IOC,
        RX_MODE_CYCLIC,
        RX_MODE_NO_SYMBOL_SEARCH,
    };

    // Bit set by hardware in FCS field when AUTO_CRC is set
    enum {
        AUTO_CRC_OK = 1 << 7,
    };

    // Useful bits in XREG_FRMCTRL1
    enum {
        PENDING_OR         = 1 << 2,
        IGNORE_TX_UNDERF   = 1 << 1,
        SET_RXENMASK_ON_TX = 1 << 0,
    };

    // Useful bits in XREG_FSMSTAT1
    enum {
        FIFO        = 1 << 7,
        FIFOP       = 1 << 6,
        SFD         = 1 << 5,
        CCA         = 1 << 4,
        SAMPLED_CCA = 1 << 3,
        LOCK_STATUS = 1 << 2,
        TX_ACTIVE   = 1 << 1,
        RX_ACTIVE   = 1 << 0,
    };

    // Useful bits in SFR_RFIRQF1
    enum {
        TXDONE = 1 << 1,
    };

    // RFIRQF0 Interrupts
    enum {
        INT_RXMASKZERO      = 1 << 7,
        INT_RXPKTDONE       = 1 << 6,
        INT_FRAME_ACCEPTED  = 1 << 5,
        INT_SRC_MATCH_FOUND = 1 << 4,
        INT_SRC_MATCH_DONE  = 1 << 3,
        INT_FIFOP           = 1 << 2,
        INT_SFD             = 1 << 1,
        INT_ACT_UNUSED      = 1 << 0,
    };

    // RFIRQF1 Interrupts
    enum {
        INT_CSP_WAIT   = 1 << 5,
        INT_CSP_STOP   = 1 << 4,
        INT_CSP_MANINT = 1 << 3,
        INT_RFIDLE     = 1 << 2,
        INT_TXDONE     = 1 << 1,
        INT_TXACKDONE  = 1 << 0,
    };

    // RFERRF Interrupts
    enum {
        INT_STROBEERR = 1 << 6,
        INT_TXUNDERF  = 1 << 5,
        INT_TXOVERF   = 1 << 4,
        INT_RXUNDERF  = 1 << 3,
        INT_RXOVERF   = 1 << 2,
        INT_RXABO     = 1 << 1,
        INT_NLOCK     = 1 << 0,
    };

    // Useful bits in MTCTRL
    enum {                  //Offset   Description                                                             Type    Value after reset
        MTCTRL_LATCH_MODE = 1 << 3, // 0: Reading MTM0 with MTMSEL.MTMSEL = 000 latches the high               RW      0
                                    // byte of the timer, making it ready to be read from MTM1. Reading
                                    // MTMOVF0 with MTMSEL.MTMOVFSEL = 000 latches the two
                                    // most-significant bytes of the overflow counter, making it possible to
                                    // read these from MTMOVF1 and MTMOVF2.
                                    // 1: Reading MTM0 with MTMSEL.MTMSEL = 000 latches the high
                                    // byte of the timer and the entire overflow counter at once, making it
                                    // possible to read the values from MTM1, MTMOVF0, MTMOVF1, and MTMOVF2.
        MTCTRL_STATE      = 1 << 2, // State of MAC Timer                                                      RO      0
                                    // 0: Timer idle
                                    // 1: Timer running
        MTCTRL_SYNC       = 1 << 1, // 0: Starting and stopping of timer is immediate; that is, synchronous    RW      1
                                    // with clk_rf_32m.
                                    // 1: Starting and stopping of timer occurs at the first positive edge of
                                    // the 32-kHz clock. For more details regarding timer start and stop,
                                    // see Section 22.4.
        MTCTRL_RUN        = 1 << 0, // Write 1 to start timer, write 0 to stop timer. When read, it returns    RW      0
                                    // the last written value.
    };

    // Useful bits in MSEL
    enum {
        MSEL_MTMOVFSEL = 1 << 4, // See possible values below
        MSEL_MTMSEL    = 1 << 0  // See possible values below
    };
    enum {
        OVERFLOW_COUNTER  = 0x00,
        OVERFLOW_CAPTURE  = 0x01,
        OVERFLOW_PERIOD   = 0x02,
        OVERFLOW_COMPARE1 = 0x03,
        OVERFLOW_COMPARE2 = 0x04,
    };
    enum {
        TIMER_COUNTER  = 0x00,
        TIMER_CAPTURE  = 0x01,
        TIMER_PERIOD   = 0x02,
        TIMER_COMPARE1 = 0x03,
        TIMER_COMPARE2 = 0x04,
    };
    enum {
        INT_PER               = 1 << 0,
        INT_COMPARE1          = 1 << 1,
        INT_COMPARE2          = 1 << 2,
        INT_OVERFLOW_PER      = 1 << 3,
        INT_OVERFLOW_COMPARE1 = 1 << 4,
        INT_OVERFLOW_COMPARE2 = 1 << 5,
        INT_MASK              = 0x3f,
    };


    // The MAC timer in CC2538
    class Timer
    {
    private:
        const static Hertz CLOCK = 32 * 1000 * 1000; // 32 MHz
        const static PPB ACCURACY = 40000; // 40 PPM

    public:
        typedef Reg64 Count;
        typedef Reg32 Interrupt_Mask;
        typedef IEEE802_15_4::Time_Stamp Time_Stamp;

    public:
        void config(const Count & count, bool interrupt, bool periodic) {
            // Clear any pending compare interrupts
            mactimer(MTIRQF) = mactimer(MTIRQF) & INT_OVERFLOW_PER;
            mactimer(MTMSEL) = (OVERFLOW_COMPARE1 * MSEL_MTMOVFSEL) | (TIMER_COMPARE1 * MSEL_MTMSEL);
            counter(count);
        }

        Time_Stamp read() {
            mactimer(MTMSEL) = (OVERFLOW_COUNTER * MSEL_MTMOVFSEL) | (TIMER_COUNTER * MSEL_MTMSEL);
            return static_cast<Time_Stamp>(counter());
        }

        Time_Stamp sfdts() {
            mactimer(MTMSEL) = (OVERFLOW_CAPTURE * MSEL_MTMOVFSEL) | (TIMER_CAPTURE * MSEL_MTMSEL);
            return static_cast<Time_Stamp>(counter());
        }

        constexpr Hertz clock() const { return CLOCK; }
        constexpr PPB accuracy() const { return ACCURACY; }

        void int_enable(const Interrupt_Mask & mask = INT_MASK) { mactimer(MTIRQM) |= mask; }
        void int_disable(const Interrupt_Mask & mask = -1U) { mactimer(MTIRQM) &= ~mask; }

        void reset()
        {
            mactimer(MTCTRL) &= ~MTCTRL_RUN;            // stop counting
            int_disable();                              // mask interrupts
            mactimer(MTIRQF) = 0;                       // clear interrupts
            mactimer(MTCTRL) &= ~MTCTRL_SYNC;           // disable use the sync feature because we want to change the count and overflow values when the timer is stopped
            mactimer(MTCTRL) |= MTCTRL_LATCH_MODE;      // count and overflow will be latched at once
            int_enable(INT_OVERFLOW_PER);               // enable overflow interrupt
            mactimer(MTCTRL) |= MTCTRL_RUN;             // start counting
        }

        Interrupt_Mask eoi(const Interrupt_Mask & mask = 0) {
            Interrupt_Mask tmp = mactimer(MTIRQF);
            mactimer(MTIRQF) = mask;
            return tmp;
        }

        static Count us2count(const Microsecond & us) { return Convert::us2count<Count, Microsecond>(CLOCK, us); }
        static Microsecond count2us(const Count & count) { return Convert::count2us<Hertz, Count, Microsecond>(CLOCK, count); }

    private:
        Count counter() {
            // MTCTRL_LATCH_MODE is set at init(), so all registers are latched at once
            Count count = mactimer(MTM0); // M0 must be read first
            count += mactimer(MTM1) << 8;
            count += static_cast<Count>(mactimer(MTMOVF0)) << 16;
            count += static_cast<Count>(mactimer(MTMOVF1)) << 24;
            count += static_cast<Count>(mactimer(MTMOVF2)) << 32;
            return count;
        }

        void counter(const Count & count) {
            mactimer(MTM0) = count;
            mactimer(MTM1) = (count >> 8);
            mactimer(MTMOVF0) = (count >> 16);
            mactimer(MTMOVF1) = (count >> 24);
            mactimer(MTMOVF2) = (count >> 32);
        }

        volatile Reg32 & mactimer (unsigned int o) { return reinterpret_cast<volatile Reg32 *>(this)[o / sizeof(Reg32)]; }
    };

public:
    CC2538RF() { }

    bool reset() {
        // Stop the radio
        sfr(RFST) = ISSTOP;
        sfr(RFST) = ISRFOFF;

        // Disable interrupts
        xreg(RFIRQM0) = 0;
        xreg(RFIRQM1) = 0;
        xreg(RFERRM) = 0;

        // Change recommended in the user guide (CCACTRL0 register description)
        xreg(CCACTRL0) = 0xf8;

        // Changes recommended in the user guide (Section 23.15 Register Settings Update)
        xreg(TXFILTCFG) = 0x09;
        xreg(AGCCTRL1) = 0x15;
        ana(IVCTRL) = 0x0b;
        xreg(FSCAL1) = 0x01;

        // Clear FIFOs
        sfr(RFST) = ISFLUSHTX;
        sfr(RFST) = ISFLUSHRX;

        // Disable receiver deafness for 192us after frame reception
        xreg(FSMCTRL) &= ~RX2RX_TIME_OFF;

        // Reset result of source matching (value undefined on reset)
        ffsm(SRCRESINDEX) = 0;

        // Set FIFOP threshold to maximum
        xreg(FIFOPCTRL) = 0xff;

        // Maximize transmission power
        xreg(TXPOWER) = 0xff;

        // Disable counting of MAC overflows
        xreg(CSPT) = 0xff;

        // Clear interrupts
        sfr(RFIRQF0) = 0;
        sfr(RFIRQF1) = 0;

        // Clear error flags
        sfr(RFERRF) = 0;

        power(FULL);
        // ACK frames are handled only when expected
        xreg(FRMFILT1) &= ~ACCEPT_FT2_ACK;

        // Enable auto-CRC
        xreg(FRMCTRL0) |= AUTO_CRC;

        if(Traits<IEEE802_15_4_NIC>::tstp_mac) {
            if(promiscuous)
                xreg(FRMFILT0) &= ~FRAME_FILTER_EN;
            else
                xreg(FRMFILT0) |= FRAME_FILTER_EN;
            xreg(SRCMATCH) |= SRC_MATCH_EN; // Enable automatic source address matching
            xreg(FRMCTRL0) |= AUTO_ACK; // Enable auto ACK
            xreg(RXMASKSET) = RXENABLE_STXON; // Enter receive mode after ISTXON
        } else {
            xreg(FRMFILT0) &= ~FRAME_FILTER_EN; // Disable frame filtering
            xreg(SRCMATCH) &= ~SRC_MATCH_EN; // Disable automatic source address matching
            if(Traits<System>::DUTY_CYCLE == 1000000)
                xreg(RXMASKSET) = RXENABLE_STXON; // Enter receive mode after ISTXON
            else
                xreg(FRMCTRL1) &= ~SET_RXENMASK_ON_TX; // Do not enter receive mode after ISTXON
        }

        channel(Traits<IEEE802_15_4_NIC>::DEFAULT_CHANNEL);

        if(Traits<IEEE802_15_4_NIC>::gpio_debug) {
            // Enable debug signals to GPIO
            GPIO p_tx('C', 3, GPIO::OUT, GPIO::FLOATING); // Configure GPIO pin C3
            GPIO p_rx('C', 5, GPIO::OUT, GPIO::FLOATING); // Configure GPIO pin C5
            if(Traits<IEEE802_15_4_NIC>::promiscuous) {
                xreg(RFC_OBS_CTRL0) = SIGNAL_RX_ACTIVE; // Signal 0 is RX_ACTIVE
                xreg(RFC_OBS_CTRL1) = SIGNAL_TX_ACTIVE; // Signal 1 is TX_ACTIVE
            } else {
                xreg(RFC_OBS_CTRL0) = SIGNAL_TX_ACTIVE; // Signal 0 is TX_ACTIVE
                xreg(RFC_OBS_CTRL1) = SIGNAL_RX_ACTIVE; // Signal 1 is RX_ACTIVE
            }
            cctest(CCTEST_OBSSEL3) = OBSSEL_SIG0_EN; // Route signal 0 to GPIO pin C3
            cctest(CCTEST_OBSSEL5) = OBSSEL_SIG1_EN; // Route signal 1 to GPIO pin C5
        }

        // Clear interrupts
        sfr(RFIRQF0) = 0;
        sfr(RFIRQF1) = 0;

        // Clear error flags
        sfr(RFERRF) = 0;

        // Enable useful device interrupts
        // INT_TXDONE, INT_TXUNDERF, and INT_TXOVERF are polled by CC2538RF::tx_done()
        // INT_RXPKTDONE is polled by CC2538RF::rx_done()
        xreg(RFIRQM0) = INT_FIFOP;
        xreg(RFIRQM1) = 0;
        xreg(RFERRM) = (INT_RXUNDERF | INT_RXOVERF);

        return true;
    }

    void address(const IEEE802_15_4::Address & address) {
        ffsm(SHORT_ADDR0) = address[0];
        ffsm(SHORT_ADDR1) = address[1];
    }

    IEEE802_15_4::Address address() {
        IEEE802_15_4::Address address;
        address[0] = ffsm(SHORT_ADDR0);
        address[1] = ffsm(SHORT_ADDR1);
        return address;
    }

    void backoff(const Microsecond & time) {
        Timer * _timer = new (reinterpret_cast<void *>(Memory_Map::RF_BASE + MACTIMER)) Timer;
        Timer::Count end = _timer->read() + _timer->us2count(time);
        while(_timer->read() <= end);
    }

    void int_disable( ) {
        Timer * _timer = new (reinterpret_cast<void *>(Memory_Map::RF_BASE + MACTIMER)) Timer;
        _timer->int_disable();
     }

    bool cca(const Microsecond & time) {
        Timer *_timer = new (reinterpret_cast<void *>(Memory_Map::RF_BASE + MACTIMER)) Timer;
        Timer::Count end = _timer->read() + _timer->us2count(time);
        while(!(xreg(RSSISTAT) & RSSI_VALID));
        bool channel_free = xreg(FSMSTAT1) & CCA;
        while((_timer->read() <= end) && (channel_free = channel_free && (xreg(FSMSTAT1) & CCA)));
        return channel_free;
    }

    void transmit_no_cca() {
        xreg(RXMASKCLR) = RXENABLE_SRXON; // Don't return to receive mode after TX
        sfr(RFST) = ISTXON;
    }

    bool transmit() {
        xreg(RXMASKCLR) = RXENABLE_SRXON; // Don't return to receive mode after TX
        sfr(RFST) = ISTXONCCA;
        return (xreg(FSMSTAT1) & SAMPLED_CCA);
    }

    bool wait_for_ack(const Microsecond & timeout, Reg8 sequence_number) {
       // Disable and clear FIFOP int. We'll poll the interrupt flag
        xreg(RFIRQM0) &= ~INT_FIFOP;
        sfr(RFIRQF0) &= ~INT_FIFOP;

        // Save radio configuration
        Reg32 saved_filter_settings = xreg(FRMFILT1);

        // Accept only ACK frames now
        if(promiscuous) // Exit promiscuous mode for now
            xreg(FRMFILT0) |= FRAME_FILTER_EN;
        xreg(FRMFILT1) = ACCEPT_FT2_ACK;

        while(!tx_done());

        // Wait for either ACK or timeout
        bool acked = false;
        Timer * _timer = new (reinterpret_cast<void *>(Memory_Map::RF_BASE + MACTIMER)) Timer;
        for(Timer::Count end = _timer->read() + _timer->us2count(timeout); (_timer->read() < end);) {
            if(sfr(RFIRQF0) & INT_FIFOP) {
                unsigned int first = xreg(RXP1_PTR);
                volatile unsigned int * rxfifo = reinterpret_cast<volatile unsigned int*>(this + RXFIFO);
                if(rxfifo[first + 3] == sequence_number) {
                    acked = true;
                    break;
                }
                drop();
                sfr(RFIRQF0) &= ~INT_FIFOP;
            }
        }

        // Restore radio configuration
        xreg(FRMFILT1) = saved_filter_settings;
        if(promiscuous)
            xreg(FRMFILT0) &= ~FRAME_FILTER_EN;
        xreg(RFIRQM0) |= INT_FIFOP;

        return acked;
    }

    void listen() { sfr(RFST) = ISRXON; }

    /*
    // TODO
    static void listen_until_timer_interrupt() {
        sfr(RFST) = ISSTOP;
        sfr(RFST) = SRXON;
        unsigned int end = Timer::_int_request_time - Timer::us2count(320) - Timer::read_raw();
        sfr(RFST) = 0x80 | ((end >> 16) & 0x1f);
        sfr(RFST) = 0xB8;
        sfr(RFST) = SRFOFF;
        sfr(RFST) = ISSTART;
    }
    */

    bool tx_done() {
        bool tx_ok = (sfr(RFIRQF1) & INT_TXDONE);
        if(tx_ok)
            sfr(RFIRQF1) &= ~INT_TXDONE;
        bool tx_error = (sfr(RFERRF) & (INT_TXUNDERF | INT_TXOVERF));
        if(tx_error)
            sfr(RFERRF) &= ~(INT_TXUNDERF | INT_TXOVERF);
        return tx_ok || tx_error;
    }

    bool rx_done() {
        bool ret = (sfr(RFIRQF0) & INT_RXPKTDONE);
        if(ret)
            sfr(RFIRQF0) &= ~INT_RXPKTDONE;
        return ret;
    }

    unsigned int channel() {
        // TODO: test!
        return 11 + (xreg(FREQCTRL) - 11) / 5;
    }

    void channel(unsigned int c) {
        assert((c > MIN_CHANNEL) && (c < MAX_CHANNEL));
        xreg(FREQCTRL) = 11 + 5 * (c - 11);
    }

    Percent tx_power() {
        // TODO: test!
        return (xreg(TXPOWER) * 100) / 0xff;
    }

    void tx_power(const Percent & p) {
        // TODO: test!
        xreg(TXPOWER) = (0xff * p) / 100;
    }

    void copy_to_nic(const void * frame, unsigned int size) {
        assert(size <= 127);
        // Clear TXFIFO
        sfr(RFST) = ISFLUSHTX;
        while(xreg(TXFIFOCNT) != 0);

        // Copy Frame to TXFIFO
        const char * f = reinterpret_cast<const char *>(frame);
        sfr(RFDATA) = size; // Phy_Header (i.e. data lenght)
        for(unsigned int i = 0; i < size - sizeof(IEEE802_15_4::Trailer); i++)
            sfr(RFDATA) = f[i];
    }

    unsigned int copy_from_nic(void * frame) {
        char * f = reinterpret_cast<char *>(frame);
        unsigned int first = xreg(RXP1_PTR);
        volatile unsigned int * rxfifo = reinterpret_cast<volatile unsigned int*>(this + RXFIFO);
        unsigned int sz = rxfifo[first]; // First byte is the length of MAC frame
        for(unsigned int i = 0; i < sz; ++i)
            f[i] = rxfifo[first + i + 1];
        drop();
        return sz;
    }

    void drop() { sfr(RFST) = ISFLUSHRX; }

    bool filter() {
        bool handle_frame = false;
        unsigned int first = xreg(RXP1_PTR);
        volatile unsigned int * rxfifo = reinterpret_cast<volatile unsigned int*>(this + RXFIFO);
        unsigned char mac_frame_size = rxfifo[first];
        unsigned int last = mac_frame_size + 1; // expected size is in first byte, which is additional.
        while (xreg(RXLAST_PTR) < last); // wait until buffer is full. RXLAST_PTR is still being updated.

        db<IEEE802_15_4>(INF) << "IEEE802_15_4::filter::first=" << first << ",last:  " << last << "size=" << mac_frame_size << ",crc=" << rxfifo[first + mac_frame_size] << endl;

        if(last - first > 1 + sizeof(IEEE802_15_4::Trailer)) {
            // On RX, last two bytes in the frame are replaced by info like CRC result
            // (obs: mac frame is preceded by one byte containing the frame length,
            // so total RXFIFO data size is 1 + mac_frame_size)
            handle_frame = (first + mac_frame_size < last) && (rxfifo[first + mac_frame_size] & AUTO_CRC_OK);
        }

        if(!handle_frame)
            drop();
        else if(Traits<IEEE802_15_4_NIC>::reset_backdoor && (mac_frame_size == sizeof(Reset_Backdoor_Message) + sizeof(IEEE802_15_4::Header))) {
            if((rxfifo[first + sizeof(IEEE802_15_4::Header) + 1] == 0xbe) && (rxfifo[first + sizeof(IEEE802_15_4::Header) + 10] == 0xef)) {
                UUID id;
                for(unsigned int i = 0; i < sizeof(UUID); i++)
                    id[i] = rxfifo[first + sizeof(IEEE802_15_4::Header) + 2 + i];
                if(id != Machine::uuid())
                    Machine::reboot();
            }
        }
        return handle_frame;
    }

    void power(const Power_Mode & mode) {
         switch(mode) {
         case ENROLL:
        	 break;
         case DISMISS:
        	 break;
         case SAME:
        	 break;
         case FULL: // Able to receive and transmit
             scr()->clock_ieee802_15_4();
             xreg(FRMCTRL0) = (xreg(FRMCTRL0) & ~(3 * RX_MODE)) | (RX_MODE_NORMAL * RX_MODE);
             xreg(RFIRQM0) &= ~INT_FIFOP;
             xreg(RFIRQM0) |= INT_FIFOP;
             break;
         case LIGHT: // Able to sense channel and transmit
             scr()->clock_ieee802_15_4();
             xreg(FRMCTRL0) = (xreg(FRMCTRL0) & ~(3 * RX_MODE)) | (RX_MODE_NO_SYMBOL_SEARCH * RX_MODE);
             break;
         case SLEEP: // Receiver off
             sfr(RFST) = ISSTOP;
             sfr(RFST) = ISRFOFF;
             xreg(RFIRQM0) &= ~INT_FIFOP;
             xreg(RFIRQF0) &= ~INT_FIFOP;
             scr()->unclock_ieee802_15_4();
             break;
         case OFF: // Radio unit shut down
             sfr(RFST) = ISSTOP;
             sfr(RFST) = ISRFOFF;
             xreg(RFIRQM0) &= ~INT_FIFOP;
             xreg(RFIRQF0) &= ~INT_FIFOP;
             scr()->unclock_ieee802_15_4();
             break;
         }
     }

    bool handle_int();

protected:
    static SysCtrl * scr() { return reinterpret_cast<SysCtrl *>(Memory_Map::SCR_BASE); }

    volatile Reg32 & rf      (unsigned int o) { return reinterpret_cast<volatile Reg32 *>(this)[o / sizeof(Reg32)]; }
    volatile Reg32 & ffsm    (unsigned int o) { return rf(o + FFSM); }
    volatile Reg32 & xreg    (unsigned int o) { return rf(o + XREG); }
    volatile Reg32 & sfr     (unsigned int o) { return rf(o + SFR); }
    volatile Reg32 & ana     (unsigned int o) { return rf(o + ANA); }
    volatile Reg32 & cctest  (unsigned int o) { return reinterpret_cast<volatile Reg32 *>(this)[(o + Memory_Map::CCTEST_BASE) / sizeof(Reg32)]; }
};


class IEEE802_15_4_Engine
{
    friend Machine;

private:
    typedef GPIO_Common::Port Port;
    typedef GPIO_Common::Pin Pin;
    typedef void (* Handler)(unsigned int);

protected:
    static const unsigned int MIN_CHANNEL = CC2538RF::MIN_CHANNEL;
    static const unsigned int MAX_CHANNEL = CC2538RF::MAX_CHANNEL;

    static const unsigned int TX_TO_RX_DELAY = CC2538RF::TX_TO_RX_DELAY; // Radio takes extra 2us to go from TX to RX or idle
    static const unsigned int RX_TO_TX_DELAY = CC2538RF::RX_TO_TX_DELAY;

    static const unsigned int SLEEP_TO_TX_DELAY = CC2538RF::SLEEP_TO_TX_DELAY;
    static const unsigned int SLEEP_TO_RX_DELAY = CC2538RF::SLEEP_TO_RX_DELAY;
    static const unsigned int TX_TO_SLEEP_DELAY = CC2538RF::TX_TO_RX_DELAY;

    static IEEE802_15_4_Engine * _instance;

public:
    // A Timer built on CC2538RF::Timer with the characteristics needed by TSTP::MAC
    class Timer
    {
    public:
        typedef CC2538RF::Timer::Count Count;
        typedef CC2538RF::Timer::Interrupt_Mask Interrupt_Mask;
        typedef IEEE802_15_4::Time_Stamp Time_Stamp;
        typedef IEEE802_15_4::Time_Stamp Offset;

    public:
        Timer() {
            db<Init, IEEE802_15_4_NIC>(TRC) << "IEEE802_15_4_Engine::Timer()" << endl;
            IC::int_vector(IC::INT_NIC0_TIMER, &timer_int_handler); // Register and enable interrupt at IC
            IC::enable(IC::INT_NIC0_TIMER);
            _offset = 0;
            _overflow_count = 0;
            _interrupts = 0;
            _handler = 0;
            _handler_activation_time = 0;
        }

        Time_Stamp sfdts() {
            CC2538RF::Timer * _timer = new (reinterpret_cast<void *>(Memory_Map::RF_BASE + CC2538RF::MACTIMER)) CC2538RF::Timer;
            return (static_cast<Time_Stamp>(_overflow_count) << 40) + static_cast<Time_Stamp>( _timer->sfdts()) + _offset;
        }

        Time_Stamp time_stamp() {
            CC2538RF::Timer * _timer = new (reinterpret_cast<void *>(Memory_Map::RF_BASE + CC2538RF::MACTIMER)) CC2538RF::Timer;
            return (static_cast<Time_Stamp>(_overflow_count) << 40) + static_cast<Time_Stamp>( _timer->read())  + _offset;
        }

        void adjust(const Offset & o) { _offset += o; }

        void handler(const Time_Stamp & when, const Handler & h) {
            CC2538RF::Timer * _timer = new (reinterpret_cast<void *>(Memory_Map::RF_BASE + CC2538RF::MACTIMER)) CC2538RF::Timer;
            _handler_activation_time = when - _offset;
            _timer->config(static_cast<Count>(_handler_activation_time), true, true);
            _interrupts = _interrupts & CC2538RF::INT_OVERFLOW_PER;
            _handler = h;
            _timer->int_enable(CC2538RF::INT_OVERFLOW_COMPARE1 | CC2538RF::INT_COMPARE1);
        }

    private:
        void handle_int(IC::Interrupt_Id interrupt);
        static void timer_int_handler(IC::Interrupt_Id interrupt);
        static void eoi(const IC::Interrupt_Id & interrupt);

    private:
        Offset _offset;
        Handler _handler;
        Time_Stamp _handler_activation_time;
        int _overflow_count;
        Interrupt_Mask _interrupts;

        //Offset _periodic_update;
        // Offset _periodic_update_update;
        // Offset _periodic_update_update_update;
        // unused?
        //bool _overflow_match;
        //bool _msb_match;
    };

protected:
    IEEE802_15_4_Engine() {
        _rf = new (reinterpret_cast<void *>(Memory_Map::RF_BASE)) CC2538RF;
        _rf->reset();
        _radio_timer = 0;
        _instance = this;
    }

public:
    IEEE802_15_4::Address address() const { return _rf->address(); }
    void address(const IEEE802_15_4::Address & address) { _rf->address(address); }

    unsigned int channel() { return _rf->channel(); }
    void channel(unsigned int c) { _rf->channel(c); }

    Percent tx_power() { return _rf->tx_power(); }
    void tx_power(const Percent & p) { _rf->tx_power(p); }

    void init() {
        _radio_timer = new (reinterpret_cast<void *>(Memory_Map::RF_BASE + CC2538RF::MACTIMER)) CC2538RF::Timer;
        _radio_timer->reset();
        _eng_timer = new Timer();
    }

    bool reset() { return _rf->reset(); };

    void backoff(const Microsecond & time) { _rf->backoff(time); }
    bool cca(const Microsecond & time) { return _rf->cca(time); }
    bool transmit() { return _rf->transmit(); }
    void transmit_no_cca() { _rf->transmit_no_cca(); }
    bool wait_for_ack(const Microsecond & timeout, int sequence_number) { return _rf->wait_for_ack(timeout, sequence_number); }
    void listen() { _rf->listen(); }
    bool tx_done() { return _rf->tx_done(); }
    bool rx_done() { return _rf->rx_done();}
    void drop() { _rf->drop(); }
    bool filter() { return _rf->filter(); }

    void copy_to_nic(const void * frame, unsigned int size) { _rf->copy_to_nic(frame, size); }
    unsigned int copy_from_nic(void * frame) { return _rf->copy_from_nic(frame); }

    void power(const Power_Mode & mode) { _rf->power(mode); }

    Timer::Time_Stamp sfdts() const { return _eng_timer->sfdts(); }
    Timer::Time_Stamp time_stamp() const { return _eng_timer->time_stamp(); }
    void timer_adjust(Timer::Offset o) const { _eng_timer->adjust(o); }
    constexpr PPB timer_accuracy() const { return _radio_timer->accuracy(); }
    constexpr Hertz timer_frequency() const { return _radio_timer->clock(); }
    Microsecond us2count( CC2538RF::Timer::Count cnt ) { return CC2538RF::Timer::count2us( cnt ); }
    CC2538RF::Timer::Count count2us( Microsecond cnt ) { return CC2538RF::Timer::us2count( cnt ); }

    void int_disable() { _rf->int_disable(); }
    bool handle_int() { return _rf->handle_int(); }
    Timer::Time_Stamp read() { return _radio_timer->read(); }
    void handler(const CC2538RF::Timer::Time_Stamp & when, const Handler & h) { _eng_timer->handler(when, h); }

    static IEEE802_15_4_Engine * instance();

private:
    void eoi(IC::Interrupt_Id interrupt) {
        IC::disable(IC::INT_NIC0_TIMER); // Make sure radio and MAC timer don't preempt one another
    }

private:
    CC2538RF * _rf;
    CC2538RF::Timer * _radio_timer;
    static Timer * _eng_timer;
};

__END_SYS

#endif

#endif
