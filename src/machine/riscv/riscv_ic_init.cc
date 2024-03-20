// EPOS RISC-V Interrupt Controller Initialization

#include <machine/ic.h>
#include <machine/timer.h>

__BEGIN_SYS

void IC::init()
{
    db<Init, IC>(TRC) << "IC::init()" << endl;

    assert(CPU::int_disabled()); // will be reenabled at Thread::init() by Context::load()

    disable(); // will be enabled on demand as handlers are registered

    // Set all exception handlers to exception()
    for(Interrupt_Id i = 0; i < EXCS; i++)
        _int_vector[i] = &exception;

    // Set all interrupt handlers to int_not()
    for(Interrupt_Id i = EXCS; i < INTS; i++)
        _int_vector[i] = &int_not;

    IC::enable(INT_PLIC);
    PLIC::threshold(0); // set the threshold to 0 so all enabled external interrupts will be dispatched
}

__END_SYS
