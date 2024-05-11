// EPOS RISC V Initialization

#include <boot_synchronizer.h>
#include <machine.h>

__BEGIN_SYS

void Machine::pre_init(System_Info * si)
{
    CPU::tvec(CPU::INT_DIRECT, &IC::entry);

    if (Boot_Synchronizer::try_acquire())
        Display::init();

    db<Init, Machine>(TRC) << "Machine::pre_init()" << endl;
}


void Machine::init()
{
    db<Init, Machine>(TRC) << "Machine::init()" << endl;

    if(Traits<IC>::enabled)
        IC::init();

    if(Traits<Timer>::enabled) {
        Timer::init();
    }
}

__END_SYS
