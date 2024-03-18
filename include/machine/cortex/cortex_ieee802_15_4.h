// EPOS ARM Cortex IEEE 802.15.4 NIC Mediator Declarations

#ifndef __cortex_ieee802_15_4_h
#define __cortex_ieee802_15_4_h

#include <utility/convert.h>
#include <network/ieee802_15_4.h>
#include <network/ieee802_15_4_mac.h>
#include <network/tstp/mac.h>
#include __HEADER_MMOD(ieee802_15_4)

__BEGIN_SYS

//class IEEE802_15_4_NIC: public NIC<IEEE802_15_4>, private IF<Traits<IEEE802_15_4_NIC>::tstp_mac, TSTP::MAC<IEEE802_15_4_Engine, Traits<System>::DUTY_CYCLE != 1000000>, IEEE802_15_4_MAC<IEEE802_15_4_Engine>>::Result
class IEEE802_15_4_NIC: public NIC<IEEE802_15_4>, private IEEE802_15_4_MAC<IEEE802_15_4_Engine>
{
    friend class Machine_Common;

private:
    using NIC<IEEE802_15_4>::Address;
    using NIC<IEEE802_15_4>::Protocol;
    using NIC<IEEE802_15_4>::Buffer;
    using NIC<IEEE802_15_4>::Configuration;
    using NIC<IEEE802_15_4>::Statistics;

    typedef IEEE802_15_4_Engine Engine;
//    typedef typename IF<Traits<IEEE802_15_4_NIC>::tstp_mac, TSTP::MAC<IEEE802_15_4_Engine, Traits<System>::DUTY_CYCLE != 1000000>, IEEE802_15_4_MAC<IEEE802_15_4_Engine>>::Result MAC;
    typedef IEEE802_15_4_MAC<IEEE802_15_4_Engine> MAC;

    static const unsigned int UNITS = Traits<IEEE802_15_4_NIC>::UNITS;
    static const bool promiscuous = Traits<IEEE802_15_4_NIC>::promiscuous;

    // Transmit and Receive Ring sizes
    static const unsigned int RX_BUFS = Traits<IEEE802_15_4_NIC>::RECEIVE_BUFFERS;

    // Size of the DMA Buffer that will hold the ring buffers
    static const unsigned int DMA_BUFFER_SIZE = RX_BUFS * sizeof(Buffer);

    // Interrupt dispatching binding
    struct Device {
        IEEE802_15_4_NIC * device;
        unsigned int interrupt;
        unsigned int error_interrupt;
    };

protected:
    IEEE802_15_4_NIC(unsigned int unit);

public:
    ~IEEE802_15_4_NIC();

    int send(const Address & dst, const Protocol & prot, const void * data, unsigned int size);
    int receive(Address * src, Protocol * prot, void * data, unsigned int size);

    Buffer * alloc(const Address & dst, const Protocol & prot, unsigned int once, unsigned int always, unsigned int payload);
    int send(Buffer * buf);
    bool drop(Buffer * buf) { return MAC::drop(buf); }
    void free(Buffer * buf);

    const Address & address() { return _configuration.address; }
    void address(const Address & address) { _configuration.address = address; Engine::address(address); }

    bool reconfigure(const Configuration * c = 0);
    const Configuration & configuration() { return _configuration; }

    const Statistics & statistics() { _statistics.time_stamp = Engine::time_stamp(); return _statistics; }

    static IEEE802_15_4_NIC * get(unsigned int unit = 0) { return get_by_unit(unit); }

private:
    void handle_int();

    static void int_handler(IC_Common::Interrupt_Id interrupt);

    static IEEE802_15_4_NIC * get_by_unit(unsigned int unit) {
        assert(unit < UNITS);
        return _devices[unit].device;
    }

    static IEEE802_15_4_NIC * get_by_interrupt(unsigned int interrupt) {
        IEEE802_15_4_NIC * tmp = 0;
        for(unsigned int i = 0; i < UNITS; i++)
            if((_devices[i].interrupt == interrupt) || (_devices[i].error_interrupt == interrupt))
                tmp = _devices[i].device;
        return tmp;
    };

    static void init(unsigned int unit);

private:
    Configuration _configuration;
    Statistics _statistics;

    Buffer * _rx_bufs[RX_BUFS];
    unsigned int _rx_cur_consume;
    unsigned int _rx_cur_produce;

    static Device _devices[UNITS];
};

__END_SYS

#endif
