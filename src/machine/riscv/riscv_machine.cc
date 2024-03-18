// EPOS RISC-V Mediator Implementation

#include <machine/machine.h>
#include <machine/display.h>

__BEGIN_SYS

void Machine::panic()
{
    CPU::int_disable();

    if(Traits<Display>::enabled)
        Display::puts("\nPANIC!\n");

    if(Traits<System>::reboot)
        reboot();
    else
        poweroff();
}

void Machine::reboot()
{
    if(Traits<System>::reboot) {
        db<Machine>(WRN) << "Machine::reboot()" << endl;

#ifdef __sifive_e__
        CPU::Reg * reset = reinterpret_cast<CPU::Reg *>(Memory_Map::AON_BASE);
        reset[0] = 0x5555;
#endif

        while(true) {
            CPU::int_disable();
            CPU::halt();
        }
    } else {
        poweroff();
    }
}

void Machine::poweroff()
{
    db<Machine>(WRN) << "Machine::poweroff()" << endl;

#ifdef __sifive_e__
        CPU::Reg * reset = reinterpret_cast<CPU::Reg *>(Memory_Map::AON_BASE);
        reset[0] = 0x5555;
#endif

        while(true) {
            CPU::int_disable();
            CPU::halt();
        }
}

__END_SYS
