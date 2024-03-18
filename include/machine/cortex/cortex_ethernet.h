// EPOS ARM Cortex Ethernet NIC Mediator Declarations

#ifndef __cortex_ethernet_h
#define __cortex_ethernet_h

#include <utility/convert.h>
#include <network/ethernet.h>
#include __HEADER_MMOD(nic)

__BEGIN_SYS

class Ethernet_NIC: public NIC<Ethernet>, private Ethernet_Engine
{
    friend class Machine_Common;

private:

    typedef Ethernet_Engine Engine;

    static const unsigned int UNITS = Traits<Ethernet>::UNITS;
    static const bool promiscuous = Traits<Ethernet>::promiscuous;     // Mode

    // Interrupt dispatching binding
    struct Device {
        Ethernet_NIC * device;
        unsigned int interrupt;
    };

protected:
    Ethernet_NIC(unsigned int unit);

public:
    ~Ethernet_NIC();

    int send(const Address & dst, const Protocol & prot, const void * data, unsigned int size);
    int receive(Address * src, Protocol * prot, void * data, unsigned int size);

    Buffer * alloc(const Address & dst, const Protocol & prot, unsigned int once, unsigned int always, unsigned int payload);
    int send(Buffer * buf);
    void free(Buffer * buf);

    const Address & address() { return _configuration.address; }
    void address(const Address & address) { _configuration.address = address; _configuration.selector = Configuration::ADDRESS; reconfigure(&_configuration); }

    bool reconfigure(const Configuration * c);
    const Configuration & configuration() { return _configuration; }

    const Statistics & statistics() {  _statistics.time_stamp = TSC::time_stamp(); return _statistics; }

    void reset();

    virtual void attach(Observer * o, const Protocol & p) {
        NIC<Ethernet>::attach(o, p);
        ; // enable receive interrupt
    }

    virtual void detach(Observer * o, const Protocol & p) {
        NIC<Ethernet>::detach(o, p);
        if(!observers())
            ; // disable receive interrupt
    }

    static Ethernet_NIC * get(unsigned int unit = 0) { return get_by_unit(unit); }

private:
    void handle_int();

    static void int_handler(IC_Common::Interrupt_Id interrupt);

    static Ethernet_NIC * get_by_unit(unsigned int unit) {
        assert(unit < UNITS);
        return _devices[unit].device;
    }

    static Ethernet_NIC * get_by_interrupt(unsigned int interrupt) {
        for(unsigned int i = 0; i < UNITS; i++)
            if(_devices[i].interrupt == interrupt)
                return _devices[i].device;
        db<Ethernet>(WRN) << "Ethernet_NIC::get_by_interrupt(" << interrupt << ") => no device bound!" << endl;
        return 0;
    };

    static void init(unsigned int unit);

private:
    Configuration _configuration;
    Statistics _statistics;

    static Device _devices[UNITS];
};

__END_SYS

#endif
