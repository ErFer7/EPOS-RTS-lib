/*
* EPOS RTL8139 PC Ethernet NIC Mediator Declarations
* Authors: Gabriel Müller, Arthur Bridi Guazzelli and Juliana Silva Pinheiro
* Year: 2019
*/

#ifndef __rtl8139_h
#define __rtl8139_h

#include <architecture.h>
#include <utility/convert.h>
#include <network/ethernet.h>

__BEGIN_SYS

class RTL8139: public NIC<Ethernet>
{
    friend class Machine_Common;

protected:
    typedef CPU::Reg8 Reg8;
    typedef CPU::Reg16 Reg16;
    typedef CPU::Reg32 Reg32;
    typedef CPU::Log_Addr Log_Addr;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::IO_Port IO_Port;
    typedef CPU::IO_Irq IO_Irq;
    typedef MMU::DMA_Buffer DMA_Buffer;
    typedef Ethernet::Address MAC_Address;

private:
    // RTL8139 registers
    enum {
        MAC0_5     = 0x00,      // IDR0 --> IDR5    | MAC
        MAR0_7     = 0x08,      // MAR0 --> MAR7    | Multicast Register
        TRSTATUS   = 0x10,      // TSD0 --> TSD3    | TX Status - 4 registers
        TRSTART    = 0x20,      // TSAD0 --> TSAD3  | TX Start Address - 4 registers
        RBSTART    = 0x30,      // RBSTART          | RX Start Address
        CMD        = 0x37,
        CAPR       = 0x38,
        CBR        = 0x3A,
        IMR        = 0x3C,      // Interrupt Mask Register
        ISR        = 0x3E,      // Interrupt Status Register
        RCR        = 0x44,      // RX Configure Register
        CONFIG_1   = 0x52,
    };

    // RTL8139 bit masks all over configuration registers ()
    enum {
        POWER_ON   = 0x00,      // CONFIG_1
        RESET      = 0x10,      // CMD
        TOK        = 0x04,      // IMR and ISR
        ROK        = 0x01,      // IMR and ISR
        RX_OVERFLOW= 0x10,      // IMR and ISR
        ACCEPT_ANY = 0x0F,      // RCR
        TE         = 0x04,      // CMD --> TX state machine on
        RE         = 0x08,      // CMD --> RX state machine on
        WRAP       = 0x80,      // RCR 
        STATUS_TOK = 0x8000,    // TRSTATUS
    };

    // TX Descriptor bits
    enum {
        OWN        = 1 << 13
    };

    // PCI ID
    static const unsigned int PCI_VENDOR_ID = 0x10EC;
    static const unsigned int PCI_DEVICE_ID = 0x8139; // RTL8139
    static const unsigned int PCI_REG_IO = 0;
    static const unsigned int UNITS = Traits<RTL8139>::UNITS;

    // Buffer config
    static const unsigned int RX_NO_WRAP_SIZE = Traits<RTL8139>::RECEIVE_BUFFERS;
    static const unsigned int RX_BUFFER_SIZE = (RX_NO_WRAP_SIZE + 16 + 1522 + 3) & ~3;
    static const unsigned int TX_BUFFER_SIZE = (sizeof(Frame) + 3) & ~3;
    static const unsigned int TX_BUFS = Traits<RTL8139>::SEND_BUFFERS;

    // Size of the DMA Buffer
    static const unsigned int DMA_BUFFER_SIZE = RX_BUFFER_SIZE + TX_BUFFER_SIZE * TX_BUFS;

    struct Device {
        RTL8139 * device;
        unsigned int interrupt;
    };

protected:
    RTL8139(unsigned int unit, IO_Port io_port, IO_Irq irq, DMA_Buffer * dma_buf);

public:
    ~RTL8139();

    int send(const Address & dst, const Protocol & prot, const void * data, unsigned int size);
    int receive(Address * src, Protocol * prot, void * data, unsigned int size);

    Buffer * alloc(const Address & dst, const Protocol & prot, unsigned int once, unsigned int always, unsigned int payload);
    int send(Buffer * buf);
    void free(Buffer * buf);

    const Address & address() { return _configuration.address; }
    void address(const Address & address) { _configuration.address = address; _configuration.selector = Configuration::ADDRESS; reconfigure(&_configuration); }

    bool reconfigure(const Configuration * c);
    const Configuration & configuration() { return _configuration; }

    const Statistics & statistics() { _statistics.time_stamp = TSC::time_stamp(); return _statistics; }

    virtual void attach(Observer * o, const Protocol & p) {
        db<RTL8139>(TRC) << "RTL8139::attach(p=" << p  << ")" << endl;
        NIC<Ethernet>::attach(o, p);
        CPU::out16 (_io_port + IMR, ROK); // enable receive int
    }

    virtual void detach(Observer * o, const Protocol & p) {
        NIC<Ethernet>::detach(o, p);
        if(!observers())
            CPU::out16(_io_port + IMR, CPU::in16(_io_port + IMR) & ~ROK); // disable receive int
    }

    static RTL8139 * get(unsigned int unit = 0) { return get_by_unit(unit); }

private:
    void reset();
    void handle_int();

    static void int_handler(IC::Interrupt_Id interrupt);

    static RTL8139 * get_by_unit(unsigned int unit) {
        assert(unit < UNITS);
        return _devices[unit].device;
    }

    static RTL8139 * get_by_interrupt(unsigned int interrupt) {
        for(unsigned int i = 0; i < UNITS; i++)
            if(_devices[i].interrupt == interrupt)
                return _devices[i].device;
        db<RTL8139>(WRN) << "RTL8139::get_by_interrupt(" << interrupt << ") => no device bound!" << endl;
        return 0;
    };

    static void init(unsigned int unit);

protected:
    IO_Port _io_port;

private:
    Configuration _configuration;
    Statistics _statistics;

    IO_Irq _irq;
    DMA_Buffer * _dma_buf;

    unsigned int _rx_read = 0; //_rx_cur
    char * _rx_base_phy;

    volatile unsigned char _tx_cur_nic = 0; // What descriptor is the NIC using?
    volatile unsigned char _tx_cur = 0; // What descriptor is the CPU using?
    char * _tx_base_phy[TX_BUFS];

    char * _rx_buffer;
    Buffer * _tx_buffer[TX_BUFS];

    static Device _devices[UNITS];
};

__END_SYS

#endif
