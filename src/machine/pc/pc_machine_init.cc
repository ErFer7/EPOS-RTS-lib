// EPOS PC Mediator Initialization

#include <machine.h>

__BEGIN_SYS

void Machine::pre_init(System_Info * si)
{
    Display::init();

    db<Init, Machine>(TRC) << "Machine::pre_init()" << endl;
}

void Machine::init()
{
    db<Init, Machine>(TRC) << "Machine::init()" << endl;

    if(Traits<IC>::enabled)
        IC::init();

    if(Traits<Timer>::enabled)
        Timer::init();

    if(Traits<PCI>::enabled)
        PCI::init();

#ifdef __SCRATCHPAD_H
    if(Traits<Scratchpad>::enabled)
        Scratchpad::init();
#endif

#ifdef __KEYBOARD_H
    if(Traits<Keyboard>::enabled)
        Keyboard::init();
#endif

#ifdef __FPGA_H
    if(Traits<FPGA>::enabled)
        FPGA::init();
#endif

#ifdef __NIC_H
#ifdef __ethernet__
    if(Traits<Ethernet>::enabled)
        Initializer<Ethernet>::init();
#endif
#endif
}

__END_SYS
