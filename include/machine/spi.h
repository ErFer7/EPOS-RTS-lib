// EPOS SPI Mediator Common package

#ifndef __spi_h
#define __spi_h

#include <system/config.h>

__BEGIN_SYS

class SPI_Common
{
public:
    enum Protocol {
        MOTO0,                  // Motorola, polarity 0, phase 0
        MOTO1,                  // Motorola, polarity 0, phase 1
        MOTO2,                  // Motorola, polarity 1, phase 0
        MOTO3,                  // Motorola, polarity 1, phase 1
        TI,                     // TI
        NMW,                    // National microwave
        Si5_SINGLE  = 0,       // SiFive-U, single SPI (DQ0[MOSI], DQ1[MISO])
        Si5_DUAL    = 1,       // SiFive-U, dual SPI (DQ0, DQ1)
        Si5_QUAD    = 2,       // SiFive-U, quad SPI (DQ0, DQ1, DQ2, DQ3)
    };

    enum Mode {
        MASTER,
        SLAVE,
        SLAVE_OD
    };

protected:
    SPI_Common() {}

public:
    void config(Hertz clock, Protocol protocol, Mode mode, unsigned int bit_rate, unsigned int data_bits);

    int get();
    bool try_get(int * data);
    void put(int data);
    bool try_put(int data);

    int read(char * data, unsigned int max_size);
    int write(const char * data, unsigned int size);

    void flush();
    bool ready_to_get();
    bool ready_to_put();

    void int_enable(bool receive = true, bool transmit = true, bool time_out = true, bool overrun = true);
    void int_disable(bool receive = true, bool transmit = true, bool time_out = true, bool overrun = true);
};

__END_SYS

#endif

#if defined(__SPI_H) && !defined(__spi_common_only__)
#include __SPI_H
#endif
