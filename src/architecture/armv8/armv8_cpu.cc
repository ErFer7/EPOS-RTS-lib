// EPOS ARMv7 CPU Mediator Implementation

#include <architecture/armv8/armv8_cpu.h>

__BEGIN_SYS

unsigned int CPU::_cpu_clock;
unsigned int CPU::_bus_clock;

void CPU::Context::save() volatile
{
    Reg _sp = sp();
    sp(this);
    push();
    sp(_sp);
    iret(); // generates the "1:" label used by push()
}

void CPU::Context::load() const volatile
{
    sp(this);
    pop();
}

void CPU::switch_context(Context ** o, Context * n)
{
    // Push the context into the stack and update "o"
    Context::push();
    *o = sp();

    // Set the stack pointer to "n" and pop the context
    sp(n);
    Context::pop();

    // Cross-domain return point used in save_regs()
    iret();
}

__END_SYS
