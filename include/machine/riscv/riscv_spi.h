// EPOS SiFive-U (RISC-V) QSPI Hardware Mediator

#ifndef __riscv_spi_h
#define __riscv_spi_h

#include <architecture/cpu.h>
#include <machine/spi.h>
#include <system/memory_map.h>

__BEGIN_SYS

class SiFive_SPI: public SPI_Common
{
    // This is a hardware object.
    // Use with something like "new (Memory_Map::SPIx_BASE) SiFive_SPI".

private:
    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg32 Reg32;

public:
    // SPI registers offsets from SPI_BASE
    enum {
        SCKDIV  = 0x00, // SCK divider
        SCKMODE = 0x04, // SCK mode
        CSID    = 0x10, // Chip select ID
        CSDEF   = 0x14, // Chip select default
        CSMODE  = 0x18, // Chip select mode
        DELAY0  = 0x28, // Delay control 0
        DELAY1  = 0x2c, // Delay control 1
        FMT     = 0x40, // Frame format
        TXDATA  = 0x48, // TX data
        RXDATA  = 0x4c, // RX data
        TXMARK  = 0x50, // TX watermark
        RXMARK  = 0x54, // RX watermark
        FCTRL   = 0x60, // Flash interface control
        FFMT    = 0x64, // Flash instruction format
        IE      = 0x70, // Interrupt enable
        IP      = 0x74, // Interrupt pending
    };

    // Useful bits from multiple registers
    enum {
        PROTO   =    2 << 0,
        MSB     =    0 << 2,
        LSB     =    1 << 2,
        DIR_RX  =    0 << 3,
        DIR_TX  =    1 << 3,
        LEN     = 0x0f << 16,
        FULL    =    1 << 31,   // TXDATA, TX FIFO full
        EMPTY   =    1 << 31,   // RXDATA, RX FIFO empty
        DATA    = 0xff << 0,
        TXWM    =    1 <<  0,   // IE/IP, TX water mark
        RXWM    =    1 <<  1    // IE/IP, RX water mark
    };

public:
    void config(Hertz clock, Protocol protocol, Mode mode, unsigned int bit_rate, unsigned int data_bits) {
        reg(SCKDIV) = ((clock / (2 * bit_rate)) - 1) & 0xfff;
        reg(FMT) = (data_bits << 16) | DIR_RX | MSB | protocol;
    }
    void config(Protocol * protocol, Mode * mode, unsigned int * bit_rate, unsigned int * data_bits) {
        *protocol = static_cast<Protocol>(reg(FMT) & PROTO);
        *mode = MASTER;
        *bit_rate = Traits<SPI>::CLOCK / (2 * (reg(SCKDIV) + 1));
        *data_bits = (reg(FMT) & LEN) >> 16;
    }

    Reg32 rxdata() { return reg(RXDATA); } // RXDATA consumes the data, so the ok() logic can't be implemented here
    Reg32 txdata() { return reg(TXDATA); }
    void txdata(Reg32 c) { reg(TXDATA) = c; }

    void int_enable(bool receive = true, bool transmit = true, bool time_out = true, bool overrun = true) {
        reg(IE) = (receive << 1) | transmit;
    }
    void int_disable(bool receive = true, bool transmit = true, bool time_out = true, bool overrun = true) {
        reg(IE) = reg(IE) & ~((receive << 1) | transmit);
    }

private:
    volatile Reg32 & reg(unsigned int o) { return reinterpret_cast<volatile Reg32 *>(this)[o / sizeof(Reg32)]; }
};


class SiFive_SPI_Engine: public SPI_Common
{
private:
    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg32 Reg32;

public:
    SiFive_SPI_Engine(SiFive_SPI * spi): _spi(spi), _rxd_pending(false), _rxd(0) {}

    void config(unsigned int clock, Protocol protocol, Mode mode, unsigned int bit_rate, unsigned int data_bits) {
        _spi->config(clock, protocol, mode, bit_rate, data_bits);
    }

    void config(Protocol * protocol, Mode * mode, unsigned int * bit_rate, unsigned int * data_bits) {
        _spi->config(protocol, mode, bit_rate, data_bits);
    }

    Reg8 rxd() { return (_rxd_pending ? _rxd : _spi->rxdata()) & SiFive_SPI::DATA; }
    bool rxd_ok() {
        _rxd = _spi->rxdata();
        _rxd_pending = !(_rxd & SiFive_SPI::EMPTY);
        return _rxd_pending;
    }

    void txd(Reg8 c) { _spi->txdata(c); }
    bool txd_ok() { return !(_spi->txdata() & SiFive_SPI::FULL); }

private:
    SiFive_SPI * _spi;
    bool _rxd_pending;
    Reg32 _rxd;
};


class SPI: private SiFive_SPI_Engine
{
private:
    static const unsigned int UNITS = Traits<SPI>::UNITS;

    static const unsigned int CLOCK = Traits<SPI>::CLOCK;

    static const unsigned int UNIT = Traits<SPI>::DEF_UNIT;
    static const Protocol PROTOCOL = static_cast<Protocol>(Traits<SPI>::DEF_PROTOCOL);
    static const Mode MODE = static_cast<Mode>(Traits<SPI>::DEF_MODE);
    static const unsigned int BIT_RATE = Traits<SPI>::DEF_BIT_RATE;
    static const unsigned int DATA_BITS = Traits<SPI>::DEF_DATA_BITS;

public:
    using SPI_Common::Protocol;
    using SPI_Common::Mode;

    using SPI_Common::MASTER;
    using SPI_Common::SLAVE;
    using SPI_Common::Si5_SINGLE;
    using SPI_Common::Si5_DUAL;
    using SPI_Common::Si5_QUAD;

public:
    SPI(unsigned int unit = UNIT, Protocol protocol = PROTOCOL, Mode mode = MODE, unsigned int bit_rate = BIT_RATE, unsigned int data_bits = DATA_BITS):
        SiFive_SPI_Engine(reinterpret_cast<SiFive_SPI *>((unit == 0) ? Memory_Map::SPI0_BASE : (unit == 1) ? Memory_Map::SPI1_BASE : Memory_Map::SPI2_BASE)), _unit(unit) {
        assert(unit < UNITS);
        config(CLOCK, protocol, mode, bit_rate, data_bits);
    }

    using SiFive_SPI_Engine::config;

    int get() { 
        while(!rxd_ok());
        return rxd();
    }
    bool try_get(int * data) {
        if(!rxd_ok())
            return false;
        *data = rxd();
        return true;
    }

    void put(int data) {
        while(!txd_ok());
        txd(data);
    }
    bool try_put(int data) {
        if(txd_ok())
            return false;
        txd(data);
        return true;
    }

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

private:
    unsigned int _unit;
};

__END_SYS

#endif
