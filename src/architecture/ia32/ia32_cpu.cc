// EPOS IA32 CPU Mediator Implementation

#include <architecture/ia32/ia32_cpu.h>
#include <architecture/ia32/ia32_mmu.h>
#include <machine/ic.h>
#include <system/memory_map.h>

__BEGIN_SYS

Hertz CPU::_cpu_clock;
Hertz CPU::_cpu_current_clock;
Hertz CPU::_bus_clock;

void CPU::Context::save() volatile
{
    // Save the running thread's context into its own stack (mostly for debugging)
    ASM("       push    %ebp                                                            \n"
        "       mov     %esp, %ebp                                                      \n"
        "       mov     8(%ebp), %esp         # SP = this                               \n"
        "       add     $44, %esp              # SP += sizeof(Context)                  \n"
        "       pushf                                                                   \n"
        "       push    %cs                                                             \n"
        "       push    4(%ebp)                # push IP                                \n"
        "       push    %eax                                                            \n"
        "       push    %ecx                                                            \n"
        "       push    %edx                                                            \n"
        "       push    %ebx                                                            \n"
        "       push    %ebp                   # push ESP                               \n"
        "       push    (%ebp)                 # push EBP                               \n"
        "       push    %esi                                                            \n"
        "       push    %edi                                                            \n"
        "       mov     %ebp, %esp                                                      \n"
        "       pop     %ebp                                                            \n"
        "       ret                                                                     \n");
}

void CPU::Context::load() const volatile
{
    // Pop the thread's context from the stack
    sp(this);
    pop();
}

void CPU::switch_context(Context * volatile * o, Context * volatile n)
{
    // Context switches always happen inside the kernel, without crossing levels
    // So the context is organized to mimic the structure of a stack involved in a same-level exception handling (RPL=CPL),
    // that is, FLAGS, CS, and IP, so IRET will understand it


    // Recover the return address from the stack and save the previously running thread's context ("o") into its stack
    Context::push();
    ASM("       mov     44(%esp), %eax          # get address of parameter 'o'          \n"
        "       mov     %esp, (%eax)            # update 'o' with the current SP        \n");

    // Restore the next thread's context ("n") from its stack
    ASM("       mov     48(%esp), %esp          # get address of parameter 'n'         \n");
    Context::pop();
}

__END_SYS
