// EPOS LM3S811 (ARM Cortex-M3) SETUP

#include <system/info.h>
#include <system/memory_map.h>
#include <architecture.h>

__USING_SYS;

extern "C" {
    void _start();
    void _int_entry();
    void _svc_handler();

    // LD eliminates this variable while performing garbage collection, that's why the used attribute.
    char __boot_time_system_info[sizeof(System_Info)] __attribute__ ((used)) = "<System_Info placeholder>"; // actual System_Info will be added by mkbi!

    // Interrupt Vector Table
    static constexpr const CPU::ISR * adjust_isr(const CPU::ISR * isr) { return reinterpret_cast<const CPU::ISR *>(reinterpret_cast<CPU::Reg>(isr) + 1); }

    // SETUP entry point is the Vector Table and resides in the .init section (not in .text), so it will be linked first and will be the first object after the ELF header in the image.
    CPU::ISR * const _vector_table[] __attribute__ ((used, section(".init"))) = {
        reinterpret_cast<const CPU::ISR *>(Memory_Map::BOOT_STACK + Traits<_SYS::Machine>::STACK_SIZE - sizeof(long)), // Stack pointer at reset
        adjust_isr(_start),             // Reset
        adjust_isr(_int_entry),         // NMI
        adjust_isr(_int_entry),         // Hard fault
        adjust_isr(_int_entry),         // Memory management fault
        adjust_isr(_int_entry),         // Bus fault
        adjust_isr(_int_entry),         // Usage fault
        adjust_isr(_int_entry),         // Reserved
        adjust_isr(_int_entry),         // Reserved
        adjust_isr(_int_entry),         // Reserved
        adjust_isr(_int_entry),         // Reserved
        adjust_isr(_svc_handler),       // SVCall
        adjust_isr(_int_entry),         // Reserved
        adjust_isr(_int_entry),         // Reserved
        adjust_isr(_int_entry),         // PendSV
        adjust_isr(_int_entry),         // Systick
        adjust_isr(_int_entry),         // IRQ0
        adjust_isr(_int_entry),         // IRQ1
        adjust_isr(_int_entry),         // IRQ2
        adjust_isr(_int_entry),         // IRQ3
        adjust_isr(_int_entry),         // IRQ4
        adjust_isr(_int_entry),         // IRQ5
        adjust_isr(_int_entry),         // IRQ6
        adjust_isr(_int_entry),         // IRQ7
        adjust_isr(_int_entry),         // IRQ8
        adjust_isr(_int_entry),         // IRQ9
        adjust_isr(_int_entry),         // IRQ10
        adjust_isr(_int_entry),         // IRQ11
        adjust_isr(_int_entry),         // IRQ12
        adjust_isr(_int_entry),         // IRQ13
        adjust_isr(_int_entry),         // IRQ14
        adjust_isr(_int_entry),         // IRQ15
        adjust_isr(_int_entry),         // IRQ16
        adjust_isr(_int_entry),         // IRQ17
        adjust_isr(_int_entry),         // IRQ18
        adjust_isr(_int_entry),         // IRQ19
        adjust_isr(_int_entry),         // IRQ20
        adjust_isr(_int_entry),         // IRQ21
        adjust_isr(_int_entry),         // IRQ22
        adjust_isr(_int_entry),         // IRQ23
        adjust_isr(_int_entry),         // IRQ24
        adjust_isr(_int_entry),         // IRQ25
        adjust_isr(_int_entry),         // IRQ26
        adjust_isr(_int_entry),         // IRQ27
        adjust_isr(_int_entry),         // IRQ28
        adjust_isr(_int_entry),         // IRQ29
        adjust_isr(_int_entry),         // IRQ30
        adjust_isr(_int_entry),         // IRQ31
        adjust_isr(_int_entry),         // IRQ32
        adjust_isr(_int_entry),         // IRQ33
        adjust_isr(_int_entry),         // IRQ34
        adjust_isr(_int_entry),         // IRQ35
        adjust_isr(_int_entry),         // IRQ36
        adjust_isr(_int_entry),         // IRQ37
        adjust_isr(_int_entry),         // IRQ38
        adjust_isr(_int_entry),         // IRQ39
        adjust_isr(_int_entry),         // IRQ40
        adjust_isr(_int_entry),         // IRQ41
        adjust_isr(_int_entry),         // IRQ42
        adjust_isr(_int_entry),         // IRQ43
        adjust_isr(_int_entry),         // IRQ44
        adjust_isr(_int_entry)          // IRQ45
    };

};
