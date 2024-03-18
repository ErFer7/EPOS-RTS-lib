// EPOS RISC-V Interrupt Controller Initialization

#include <architecture/cpu.h>
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

    // Install the syscall trap handler
#ifdef __kernel__
    _int_vector[INT_SYSCALL] = &CPU::syscalled;
#endif

    // Set all interrupt handlers to int_not()
    for(Interrupt_Id i = EXCS; i < INTS; i++)
        _int_vector[i] = &int_not;

    enable(INT_PLIC); // enable PLIC interrupt (external interrupts)
    PLIC::threshold(0); // set the threshold to 0 so all enabled external interrupts will be dispatched
}

__END_SYS
