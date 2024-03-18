// EPOS Zynq-7000 (Cortex-A9) NIC Mediator Declarations

#ifndef __zynq_nic_h
#define __zynq_nic_h

#include <machine/nic.h>
#include <machine/cortex/engine/cortex_a9/gem.h>

__BEGIN_SYS

class Ethernet_Engine: private GEM
{
protected:
    Ethernet_Engine(unsigned int unit);

public:
    ~Ethernet_Engine();

    void reset();

private:
    void handle_int();

    static void int_handler(IC_Common::Interrupt_Id interrupt);

    static void init(unsigned int unit);

private:

};

__END_SYS

#endif
