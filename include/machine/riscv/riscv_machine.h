// EPOS RISC-V Mediator Declarations

#ifndef __riscv_machine_h
#define __riscv_machine_h

#include <architecture.h>
#include <machine/machine.h>
#include <machine/ic.h>
#include <machine/display.h>
#include <system/info.h>
#include <system/memory_map.h>
#include <system.h>

__BEGIN_SYS

class Machine: private Machine_Common
{
    friend class Setup;
    friend class Init_Begin;
    friend class Init_System;

public:
    Machine() {}

    using Machine_Common::delay;
    using Machine_Common::clear_bss;

    static void panic();
    static void reboot();
    static void poweroff();

    static const UUID & uuid() { return System::info()->bm.uuid; }

private:
    static void pre_init(System_Info * si);
    static void init();
};

__END_SYS

#endif
