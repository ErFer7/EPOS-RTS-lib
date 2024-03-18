// EPOS RISC-V 32 CPU Mediator Implementation

#include <architecture/rv32/rv32_cpu.h>
#include <system.h>

__BEGIN_SYS

unsigned int CPU::_cpu_clock;
unsigned int CPU::_bus_clock;

void CPU::Context::save() volatile
{
    ASM("       sw       x1,    0(a0)           \n"     // push RA as PC
        "       csrr     x3,  mstatus           \n"
        "       sw       x3,    4(a0)           \n"     // push ST
        "       sw       x1,    8(a0)           \n"     // push X1-X31
        "       sw       x5,   12(a0)           \n"
        "       sw       x6,   16(a0)           \n"
        "       sw       x7,   20(a0)           \n"
        "       sw       x8,   24(a0)           \n"
        "       sw       x9,   28(a0)           \n"
        "       sw      x10,   32(a0)           \n"
        "       sw      x11,   36(a0)           \n"
        "       sw      x12,   40(a0)           \n"
        "       sw      x13,   44(a0)           \n"
        "       sw      x14,   48(a0)           \n"
        "       sw      x15,   52(a0)           \n"
        "       sw      x16,   56(a0)           \n"
        "       sw      x17,   60(a0)           \n"
        "       sw      x18,   64(a0)           \n"
        "       sw      x19,   68(a0)           \n"
        "       sw      x20,   72(a0)           \n"
        "       sw      x21,   76(a0)           \n"
        "       sw      x22,   80(a0)           \n"
        "       sw      x23,   84(a0)           \n"
        "       sw      x24,   88(a0)           \n"
        "       sw      x25,   92(a0)           \n"
        "       sw      x26,   96(a0)           \n"
        "       sw      x27,  100(a0)           \n"
        "       sw      x28,  104(a0)           \n"
        "       sw      x29,  108(a0)           \n"
        "       sw      x30,  112(a0)           \n"
        "       sw      x31,  116(a0)           \n"
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
    ASM("sw sp, 0(a0)");   // update Context * volatile * o, which is in a0

    // Set the stack pointer to "n" and pop the context from the stack
    ASM("mv sp, a1");   // "n" is in a1
    Context::pop();
    iret();
}

__END_SYS

