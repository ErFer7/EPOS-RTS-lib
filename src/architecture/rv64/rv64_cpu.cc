// EPOS RISC-V 64 CPU Mediator Implementation

#include <architecture/rv64/rv64_cpu.h>
#include <system.h>

__BEGIN_SYS

unsigned int CPU::_cpu_clock;
unsigned int CPU::_bus_clock;

void CPU::Context::save() volatile
{
    ASM("       sd       x1,    0(a0)           \n"     // push RA as PC
        "       csrr     x3,  mstatus           \n"
        "       sd       x3,    8(a0)           \n"     // push ST
        "       sd       x1,   16(a0)           \n"     // push X1-X31
        "       sd       x5,   24(a0)           \n"
        "       sd       x6,   32(a0)           \n"
        "       sd       x7,   40(a0)           \n"
        "       sd       x8,   48(a0)           \n"
        "       sd       x9,   56(a0)           \n"
        "       sd      x10,   64(a0)           \n"
        "       sd      x11,   72(a0)           \n"
        "       sd      x12,   80(a0)           \n"
        "       sd      x13,   88(a0)           \n"
        "       sd      x14,   96(a0)           \n"
        "       sd      x15,  104(a0)           \n"
        "       sd      x16,  112(a0)           \n"
        "       sd      x17,  120(a0)           \n"
        "       sd      x18,  128(a0)           \n"
        "       sd      x19,  136(a0)           \n"
        "       sd      x20,  144(a0)           \n"
        "       sd      x21,  152(a0)           \n"
        "       sd      x22,  160(a0)           \n"
        "       sd      x23,  168(a0)           \n"
        "       sd      x24,  176(a0)           \n"
        "       sd      x25,  184(a0)           \n"
        "       sd      x26,  192(a0)           \n"
        "       sd      x27,  200(a0)           \n"
        "       sd      x28,  208(a0)           \n"
        "       sd      x29,  216(a0)           \n"
        "       sd      x30,  224(a0)           \n"
        "       sd      x31,  232(a0)           \n"
        "       ret                             \n");
}

// Context load does not verify if interrupts were previously enabled by the Context's constructor
// We are setting mstatus to MPP | MPIE, therefore, interrupts will be enabled only after mret
void CPU::Context::load() const volatile
{
    sp(Log_Addr(this));
    pop();
    iret();
}

void CPU::switch_context(Context ** o, Context * n)     // "o" is in a0 and "n" is in a1
{   
    // Push the context into the stack and update "o"
    Context::push();
    ASM("sd sp, 0(a0)");   // update Context * volatile * o, which is in a0

    // Set the stack pointer to "n" and pop the context from the stack
    ASM("mv sp, a1");   // "n" is in a1
    Context::pop();
    iret();
}

__END_SYS

