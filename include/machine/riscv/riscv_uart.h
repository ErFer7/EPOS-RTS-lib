// EPOS RISC-V UART Mediator Declarations

#ifndef __riscv_uart_h
#define __riscv_uart_h

#include <architecture/cpu.h>
#include <machine/uart.h>
#include <system/memory_map.h>

__BEGIN_SYS

class SiFive_UART
{
private:
    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg32 Reg32;

public:
    static const unsigned int UNITS = Traits<UART>::UNITS;
    static const unsigned int CLOCK = Traits<UART>::CLOCK;

    // UART registers offsets from UART_BASE
    enum {
        TXDATA  = 0x00,
        RXDATA  = 0x04,
        TXCTRL  = 0x08,
        RXCTRL  = 0x0c,
        IE      = 0x10,
        IP      = 0x14,
        DIV     = 0x18  // f(baud) = f(in) / (DIV + 1)
    };

    // Useful bits from multiple registers
    enum {
        FULL    =    1 << 31,   // TXDATA, TX FIFO full
        EMPTY   =    1 << 31,   // RXDATA, RX FIFO empty
        DATA    = 0xff << 0,
        TXEN    =    1 <<  0,   // TXCTRL, TX enable
        NSTOP   =    1 <<  1,   // TXCTRL, stop bits (0 -> 1 or 1 -> 2)
        TXCNT   =    7 << 16,   // TXCTRL, TX interrupt threshold (RXWM = (len(FIFO) < TXCNT))
        RXEN    =    1 <<  0,   // RXCTRL, RX enable
        RXCNT   =    7 << 16,   // RXCTRL, TXinterrupt threshold (TXWM = (len(FIFO) > RXCNT))
        TXWM    =    1 <<  0,   // IE/IP, TX water mark
        RXWM    =    1 <<  1    // IE/IP, RX water mark
    };

public:
    SiFive_UART(unsigned int unit, unsigned int baud_rate, unsigned int data_bits, unsigned int parity, unsigned int stop_bits): _rxd_pending(false), _rxd(0) {
        assert(data_bits = 8);
        config(baud_rate, data_bits, parity, stop_bits);
    }

    void config(unsigned int baud_rate, unsigned int data_bits, unsigned int parity, unsigned int stop_bits) {
        reg(TXCTRL) = 1 << 16 | stop_bits | TXEN; // TXCNT = 1, STOP = (stop_bits - 1) << 1
        reg(RXCTRL) = 1 << 16 | RXEN; // RXCNT = 1
        reg(DIV) = ((CLOCK / baud_rate) - 1) & 0xffff;
    }

    void config(unsigned int * baud_rate, unsigned int * data_bits, unsigned int * parity, unsigned int * stop_bits) {
        *baud_rate = CLOCK / (reg(DIV) + 1);
        *data_bits = 8;
        *parity = UART_Common::NONE;
        *stop_bits = ((reg(TXCTRL) & NSTOP) >> 1) + 1;
    }

    Reg8 rxd() { return (_rxd_pending ? _rxd : reg(RXDATA)) & DATA; }
    void txd(Reg8 c) { reg(TXDATA) = c & DATA; }

    bool rxd_ok() {
        _rxd = reg(RXDATA);
        _rxd_pending = !(_rxd & EMPTY);
        return _rxd_pending;
    }
    bool txd_ok() { return !(reg(TXDATA) & FULL); }

    void int_enable(bool receive = true, bool transmit = true, bool line = true, bool modem = true) {
         reg(IE) = (receive << 1) | transmit;
    }
    void int_disable(bool receive = true, bool transmit = true, bool line = true, bool modem = true) {
         reg(IE) = reg(IE) & ~((receive << 1) | transmit);
    }

    void flush() { while(!(reg(IP) & TXWM)); }

private:
    bool _rxd_pending;
    Reg32 _rxd;

    static volatile CPU::Reg32 & reg(unsigned int o) { return reinterpret_cast<volatile CPU::Reg32 *>(Memory_Map::UART0_BASE)[o / sizeof(CPU::Reg32)]; }
};

class NS16500A
{
private:
    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg32 Reg32;

    static const unsigned int UNITS = Traits<UART>::UNITS;
    static const unsigned int CLOCK = Traits<UART>::CLOCK / 16; // reference clock is pre-divided by 16

public:
    // UART registers offsets from UART_BASE
    enum {
        DIV_LSB         = 0,
        TXD             = 0,
        RXD             = 0,
        IER             = 1,
        DIV_MSB         = 1,
        FCR             = 2,
        ISR             = 2,
        LCR             = 3,
        MCR             = 4,
        LSR             = 5
    };

    // Useful bits from multiple registers
    enum {
        DATA_READY          = 1 << 0,
        THOLD_REG           = 1 << 5,
        TEMPTY_REG          = 1 << 6,
        DATA_BITS_MASK      = 1 << 1 | 1 << 0,
        PARITY_MASK         = 1 << 3,
        DLAB_ENABLE         = 1 << 7,
        STOP_BITS_MASK      = 1 << 2,
        LOOPBACK_MASK       = 1 << 4,
        FIFO_ENABLE         = 1 << 0,
        DEFAULT_DATA_BITS   = 5
    };

public:
    NS16500A(unsigned int unit, unsigned int baud_rate, unsigned int data_bits, unsigned int parity, unsigned int stop_bits) {
        assert(unit < UNITS);
        config(baud_rate, data_bits, parity, stop_bits);
    }

    void config(unsigned int baud_rate, unsigned int data_bits, unsigned int parity, unsigned int stop_bits) {
        // Disable all interrupts
        reg(IER) = 0;

        // Set clock divisor
        unsigned int div = CLOCK / baud_rate;
        dlab(true);
        reg(DIV_LSB) = div;
        reg(DIV_MSB) = div >> 8;
        dlab(false);

        // Set data word length (5, 6, 7 or 8)
        Reg8 lcr = data_bits - 5;

        // Set parity (0 [no], 1 [odd], 2 [even])
        if(parity) {
            lcr |= PARITY_MASK;
            lcr |= (parity - 1) << 4;
        }

        // Set stop-bits (1, 2 or 3 [1.5])
        lcr |= (stop_bits > 1) ? STOP_BITS_MASK : 0;

        reg(LCR) = lcr;

        // Enables Tx and Rx FIFOs, clear them, set trigger to 14 bytes
        reg(FCR) = 0xc7;

        // Set DTR, RTS and OUT2 of MCR
        reg(MCR) = reg(MCR) | 0x0b;
    }

    void config(unsigned int * baud_rate, unsigned int * data_bits, unsigned int * parity, unsigned int * stop_bits) {
        // Get clock divisor
        dlab(true);
        *baud_rate = CLOCK / (Reg32(reg(DIV_MSB) << 8) | Reg32(reg(DIV_LSB)));
        dlab(false);

        *data_bits = Reg32(reg(LCR) & DATA_BITS_MASK) + DEFAULT_DATA_BITS;

        *parity = (Reg32(reg(LCR)) & PARITY_MASK) >> PARITY_MASK;

        *stop_bits = (Reg32(reg(LCR) & STOP_BITS_MASK) >> STOP_BITS_MASK) + 1;
    }

    Reg8 rxd() { return reg(RXD); }
    void txd(Reg8 c) { reg(TXD) = c; }

    bool rxd_ok() { return (reg(LSR) & DATA_READY); }
    bool txd_ok() {  return (reg(LSR) & THOLD_REG); }

    void int_enable(bool receive = true, bool transmit = true, bool line = true, bool modem = true) {
        reg(IER) = receive | (transmit << 1) | (line << 2) | (modem << 3);
    }
    void int_disable(bool receive = true, bool transmit = true, bool line = true, bool modem = true) {
        reg(IER) = reg(IER) & ~(receive | (transmit << 1) | (line << 2) | (modem << 3));
    }

    void flush() { while(!(reg(LSR) & TEMPTY_REG)); }

    void reset() {
        // Reconfiguring the UART implicitly resets it
        unsigned int b, db, p, sb;
        config(&b, &db, &p, &sb);
        config(b, db, p, sb);
    }

    void loopback(bool flag) {
        Reg8 mask = 0xff;
        mask -= LOOPBACK_MASK;
        mask += (flag << 4); // if 1, restore flag, else make it disable loopback
        reg(MCR) = reg(MCR) & mask; 
    }

private:
    void dlab(bool f) { reg(LCR) = (reg(LCR) & 0x7f) | (f << 7); }

    static volatile CPU::Reg8 & reg(unsigned int o) { return reinterpret_cast<volatile CPU::Reg8 *>(Memory_Map::UART0_BASE)[o / sizeof(CPU::Reg8)]; }
};

class UART: private UART_Common, private IF<(Traits<Build>::MODEL == Traits<Build>::SiFive_E) || (Traits<Build>::MODEL == Traits<Build>::SiFive_U), SiFive_UART, NS16500A>::Result
{
private:
    static const unsigned int UNIT = Traits<UART>::DEF_UNIT;
    static const unsigned int BAUD_RATE = Traits<UART>::DEF_BAUD_RATE;
    static const unsigned int DATA_BITS = Traits<UART>::DEF_DATA_BITS;
    static const unsigned int PARITY = Traits<UART>::DEF_PARITY;
    static const unsigned int STOP_BITS = Traits<UART>::DEF_STOP_BITS;

    typedef IF<(Traits<Build>::MODEL == Traits<Build>::SiFive_E) || (Traits<Build>::MODEL == Traits<Build>::SiFive_U), SiFive_UART, NS16500A>::Result Engine;

public:
    using UART_Common::NONE;
    using UART_Common::EVEN;
    using UART_Common::ODD;

public:
    UART(unsigned int unit = UNIT, unsigned int baud_rate = BAUD_RATE, unsigned int data_bits = DATA_BITS, unsigned int parity = PARITY, unsigned int stop_bits = STOP_BITS)
    : Engine(unit, baud_rate, data_bits, parity, stop_bits) {}

    using Engine::config;

    char get() { while(!rxd_ok()); return rxd(); }
    void put(char c) { while(!txd_ok()); txd(c); }

    int read(char * data, unsigned int max_size) {
        for(unsigned int i = 0; i < max_size; i++)
            data[i] = get();
        return 0;
    }
    int write(const char * data, unsigned int size) {
        for(unsigned int i = 0; i < size; i++)
            put(data[i]);
        return 0;
    }

    bool ready_to_get() { return rxd_ok(); }
    bool ready_to_put() { return txd_ok(); }

    using Engine::int_enable;
    using Engine::int_disable;
    using Engine::flush;

    void power(const Power_Mode & mode);
};

__END_SYS

#endif
