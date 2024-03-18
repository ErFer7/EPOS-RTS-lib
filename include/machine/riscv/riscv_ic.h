// EPOS RISC-V IC Mediator Declarations

#ifndef __riscv_ic_h
#define __riscv_ic_h

#include <architecture/cpu.h>
#include <machine/ic.h>
#include <system/memory_map.h>

__BEGIN_SYS

// Core Local Interrupter (CLINT)
class CLINT
{
private:
    typedef CPU::Reg Reg;
    typedef CPU::Reg32 Reg32;
    typedef CPU::Reg64 Reg64;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;

    static const bool supervisor = Traits<Machine>::supervisor;
    static const unsigned long CPU_OFFSET = Traits<Machine>::CPU_OFFSET;

public:
    static const unsigned int IRQS = 16;

    // Interrupts ([M|S]CAUSE with interrupt = 1)
    enum : unsigned long {
        IRQ_USR_SOFT    = 0,
        IRQ_SUP_SOFT    = 1,
        IRQ_MAC_SOFT    = 3,
        IRQ_USR_TIMER   = 4,
        IRQ_SUP_TIMER   = 5,
        IRQ_MAC_TIMER   = 7,
        IRQ_USR_EXT     = 8,
        IRQ_SUP_EXT     = 9,
        IRQ_MAC_EXT     = 11,
        IRQ_PLIC        = (supervisor ? IRQ_SUP_EXT: IRQ_MAC_EXT),
        INTERRUPT       = 1UL << (Traits<CPU>::WORD_SIZE - 1),
        INT_MASK        = ~INTERRUPT

    };

    // Registers offsets from CLINT_BASE (all registers are 32 bits wide; 32-bit word pairs, like MTIMECMP, can be accessed as a 64-bit word in RV64)
    enum {                        // Description
        MSIP            = 0x0000, // Per-HART 32-bit software interrupts (IPI) trigger; each HART is offset by 4 bytes from MSIP
        MTIMECMP        = 0x4000, // Per-HART 64-bit timer compare register; lower 32 bits stored first; each HART is offset by 8 bytes from MTIMECMP
        MTIME           = 0xbff8, // Timer counter shared by all HARTs lower 32 bits stored first
    };

    // MTVEC modes
    enum Mode {
        DIRECT  = 0,
        INDEXED = 1
    };

public:
    static void mtvec(Mode mode, Phy_Addr base) {
        Reg tmp = (base & -4UL) | (Reg(mode) & 0x3);
        ASM("csrw mtvec, %0" : : "r"(tmp) : "cc");
    }

    static Reg mtvec() {
        Reg value;
        ASM("csrr %0, mtvec" : "=r"(value) : : );
        return value;
    }

    static void stvec(Mode mode, Log_Addr base) {
        Reg tmp = (base & -4UL) | (Reg(mode) & 0x3);
        ASM("csrw stvec, %0" : : "r"(tmp) : "cc");
    }

    static Reg stvec() {
        Reg value;
        ASM("csrr %0, stvec" : "=r"(value) : : );
        return value;
    }

    static Reg64 mtime() { return *reinterpret_cast<Reg64 *>(Memory_Map::CLINT_BASE + MTIME); }
    static void  mtimecmp(Reg64 v) { *reinterpret_cast<Reg64 *>(Memory_Map::CLINT_BASE + MTIMECMP + 8 * (CPU::id() + CPU_OFFSET)) = v; }

    static volatile Reg32 & msip(unsigned int cpu) { return *reinterpret_cast<volatile Reg32 *>(Memory_Map::CLINT_BASE + MSIP + 4 * (cpu + CPU_OFFSET)); }
};


// Platform-Level Interrupt Controller (PLIC)
class PLIC: public Traits<IC>::Interrupt_Source
{
private:
    typedef CPU::Reg32 Reg32;

    static const bool supervisor = Traits<Machine>::supervisor;
    static const unsigned long CPU_OFFSET = Traits<Machine>::CPU_OFFSET;

public:
    static const unsigned int IRQS = Traits<IC>::PLIC_IRQS;

    // Registers offsets from PLIC_BASE (all registers are 32 bits wide; 32-bit word pairs, like PENDING, can be accessed as a 64-bit word in RV64)
    enum {                              // Description
        PRIORITY        = 0x000000,     // Interrupt Source Priority (RW), 32 bits per source, up to 1024 sources;  0 => disable, 1 => lowest, .... 7 => highest
        PENDING         = 0x001000,     // Interrupt Pending Register (RO), 1 bit per source, up to 1024 sources (bit 0 is always 0, since source 0 does not exist)
        ENABLED         = 0x002000,     // per-context Interrupt Enable Register (RW), 1 bit per source, up to 1024 sources; contexts offset by 0x80 bytes
        THRESHOLD       = 0x200000,     // per-context Interrupt Priority Threshold Register (RW), 3 bits (priority 1 to 7), interrupts bellow the threshold are masked; contexts offset by 0x10000
        CLAIM           = 0x200004,     // per-context Claim/Complete Register (RW), 32 bits containing the highest interrupt source id; id == 0 => no pending interrupts; write ID to Complete (i.e. EOI); contexts offset by 0x10000
    };

public:
    static void enable() { for(Reg32 id = 1; id < IRQS; id++) enable(id); }
    static void enable(Reg32 id) { _enable(context(), id); }
    static void disable() { for(Reg32 id = 1; id < IRQS; id++) disable(id); }
    static void disable(Reg32 id) { _disable(context(), id); }

    static Reg32 claim() {
        if(!_claimed)
            _claimed = _claim(context());
        return _claimed;
    }
    static void complete(Reg32 id) {
        if(id != _claimed)
            db<IC>(WRN) << "IC::complete(id=" << id << "): completing unclaimed interrupt!" << endl;
        _complete(context(), id);
        _claimed = 0;
    }

    static Reg32 threshold() { return _threshold(context()); }
    static void threshold(Reg32 v) { _threshold(context(), v); }

    static Reg32 priority(Reg32 id) { return _priority(id); }
    static void priority(Reg32 id, Reg32 v) { _priority(id, v); }

private:
    static void _enable(Reg32 context, Reg32 id) { enabled(context, id) |= (1 << (id % 32)); }
    static void _disable(Reg32 context, Reg32 id) { enabled(context, id) &= ~(1 << (id % 32 )); }

    static Reg32 _claim(Reg32 context) { return reg(CLAIM + context * 0x1000); }
    static void _complete(Reg32 context, Reg32 id) { reg(CLAIM + context * 0x1000) = id; } // completing an inactive IRQ fails silently, so claim()/complete() must be always paired to the same id

    static Reg32 _threshold(Reg32 context) { return reg(THRESHOLD + context * 0x1000); }
    static void _threshold(Reg32 context, Reg32 v) { reg(THRESHOLD + context * 0x1000) = v; }

    static Reg32 _priority(Reg32 id) { return reg(PRIORITY + id * 4); }
    static void _priority(Reg32 id, Reg32 v) { reg(PRIORITY + id * 4) = v & 7; }

    static bool _pending(Reg32 id) { return reg(PENDING + (id >> 3)) & ~(1 << (id % 32)); }

    // SiFive-U has 9 contexts: Hart0 MAC, Hart1 MAC, Hart1 SUP, Hart2 MAC, Hart2 SUP, Hart3 MAC, Hart3 SUP, Hart4 MAC, Hart4 SUP
    static unsigned int context() { return (Traits<Build>::MODEL == Traits<Build>::SiFive_U) ? (supervisor || ((CPU::id() + CPU_OFFSET) == 0)) + (CPU::id() + CPU_OFFSET) * 2 - 1 : 0; }

    static volatile Reg32 & reg(unsigned int o) { return reinterpret_cast<volatile CPU::Reg32 *>(Memory_Map::PLIC_BASE)[o / sizeof(CPU::Reg32)]; }
    static volatile Reg32 & enabled(Reg32 context, Reg32 id) { return reg(ENABLED + context * 0x80 + (id >> 3)); } // if contexto ranges from 0 to 8

private:
    static Reg32 _claimed;
};

class IC: private IC_Common, private CLINT, private PLIC
{
    friend class Setup;
    friend class Machine;

private:
    typedef CPU::Reg Reg;

    static const bool multitask = Traits<System>::multitask;
    static const bool supervisor = Traits<Machine>::supervisor;
    static const unsigned long CPU_OFFSET = Traits<Machine>::CPU_OFFSET;

public:
    static const unsigned int EXCS = CPU::EXCEPTIONS;
    static const unsigned int IRQS = CLINT::IRQS + PLIC::IRQS;
    static const unsigned int INTS = EXCS + IRQS;

    using IC_Common::Interrupt_Id;
    using IC_Common::Interrupt_Handler;

    enum {
        INT_SYSCALL     = CPU::EXC_ENVU,
        INT_SYS_TIMER   = EXCS + (supervisor ? IRQ_SUP_TIMER : IRQ_MAC_TIMER),
        INT_RESCHEDULER = EXCS + (supervisor ? IRQ_SUP_SOFT : IRQ_MAC_SOFT),  // IPI are called software interrupts in RISC-V
        INT_PLIC        = EXCS + IRQ_PLIC,
        HARD_INT        = EXCS + CLINT::IRQS,
        INT_DDR         = HARD_INT + PLIC::IRQ_DDR,
        INT_DMA0        = HARD_INT + PLIC::IRQ_DMA0,
        INT_ETH0        = HARD_INT + PLIC::IRQ_ETH0,
        INT_GPIO0       = HARD_INT + PLIC::IRQ_GPIO0,
        INT_I2C         = HARD_INT + PLIC::IRQ_I2C,
        INT_L2_CACHE    = HARD_INT + PLIC::IRQ_L2_CACHE,
        INT_MIS0        = HARD_INT + PLIC::IRQ_MSI0,
        INT_PWM0        = HARD_INT + PLIC::IRQ_PWM0,
        INT_PWM1        = HARD_INT + PLIC::IRQ_PWM1,
        INT_PWM2        = HARD_INT + PLIC::IRQ_PWM2,
        INT_PWM3        = HARD_INT + PLIC::IRQ_PWM3,
        INT_QSPI0       = HARD_INT + PLIC::IRQ_QSPI0,
        INT_QSPI1       = HARD_INT + PLIC::IRQ_QSPI1,
        INT_QSPI2       = HARD_INT + PLIC::IRQ_QSPI2,
        INT_QSPI3       = HARD_INT + PLIC::IRQ_QSPI3,
        INT_RTC         = HARD_INT + PLIC::IRQ_RTC,
        INT_SPI0        = HARD_INT + PLIC::IRQ_SPI0,
        INT_SPI1        = HARD_INT + PLIC::IRQ_SPI1,
        INT_SPI2        = HARD_INT + PLIC::IRQ_SPI2,
        INT_SPI3        = HARD_INT + PLIC::IRQ_SPI3,
        INT_UART0       = HARD_INT + PLIC::IRQ_UART0,
        INT_UART1       = HARD_INT + PLIC::IRQ_UART1,
        INT_UART2       = HARD_INT + PLIC::IRQ_UART2,
        INT_UART3       = HARD_INT + PLIC::IRQ_UART3,
        INT_WDOG        = HARD_INT + PLIC::IRQ_WDOG
    };

public:
    IC() {}

    static Interrupt_Handler int_vector(Interrupt_Id i) {
        assert(i < INTS);
        return _int_vector[i];
    }

    static void int_vector(Interrupt_Id i, const Interrupt_Handler & h) {
        db<IC>(TRC) << "IC::int_vector(int=" << i << ",h=" << reinterpret_cast<void *>(h) << ")" << endl;
        assert(i < INTS);
        _int_vector[i] = h;
    }

    static void enable() {
        db<IC>(TRC) << "IC::enable()" << endl;
        if(supervisor)
            CPU::sie(CPU::SSI | CPU::STI | CPU::SEI);
        else
            CPU::mie(CPU::MSI | CPU::MTI | CPU::MEI);
        PLIC::enable();
    }

    static void enable(Interrupt_Id i) {
        db<IC>(TRC) << "IC::enable(int=" << i << ")" << endl;
        assert(i < INTS);
        if(i == INT_RESCHEDULER) {
            if(supervisor)
                CPU::sies(CPU::SSI);
            else
                CPU::mies(CPU::MSI);
        } else if(i == INT_SYS_TIMER) {
            if(supervisor)
                CPU::sies(CPU::STI);
            else
                CPU::mies(CPU::MTI);
        } else if(i == INT_PLIC) {
            if(supervisor)
                CPU::sies(CPU::SEI);
            else
                CPU::mies(CPU::MEI);
        } else if(i > HARD_INT) {
            i = int2irq(i);
            PLIC::enable(i);
            PLIC::priority(i, 1);
        }
    }

    static void disable() {
        db<IC>(TRC) << "IC::disable()" << endl;
        if(supervisor)
            CPU::siec(CPU::SSI | CPU::STI | CPU::SEI);
        else
            CPU::miec(CPU::MSI | CPU::MTI | CPU::MEI);
        PLIC::disable();
    }

    static void disable(Interrupt_Id i) {
        db<IC>(TRC) << "IC::disable(int=" << i << ")" << endl;
        assert(i < INTS);
        if(i == INT_RESCHEDULER) {
             if(supervisor)
                 CPU::siec(CPU::SSI);
             else
                 CPU::miec(CPU::MSI);
         } else if(i == INT_SYS_TIMER) {
             if(supervisor)
                 CPU::siec(CPU::STI);
             else
                 CPU::miec(CPU::MTI);
         } else if(i == INT_PLIC) {
             if(supervisor)
                 CPU::siec(CPU::SEI);
             else
                 CPU::miec(CPU::MEI);
         } else if(i > HARD_INT) {
             i = int2irq(i);
             PLIC::disable(i);
             PLIC::priority(i, 0);
         }
      }

    static void priority(Interrupt_Id i, unsigned int p) {
        assert((HARD_INT < i) && (i < INTS) && (p <= 7));
        PLIC::priority(int2irq(i), p);
    }

    static Interrupt_Id int_id() {
        // Id is retrieved from [m|s]cause even if mip has the equivalent bit up, because only [m|s]cause can tell if it is an interrupt or an exception
        Reg id = (supervisor) ? CPU::scause() : CPU::mcause();
        if(id & INTERRUPT)
            return irq2int(id & INT_MASK);
        else
            return (id & INT_MASK);
    }

    static int irq2int(int i) { return ((i == IRQ_PLIC) ? claim() + CLINT::IRQS : i) + EXCS; }
    static int int2irq(int i) { return  ((i > HARD_INT) ? i - CLINT::IRQS : i) - EXCS; }

    static void ipi(unsigned int cpu, Interrupt_Id i) {
        db<IC>(TRC) << "IC::ipi(cpu=" << cpu << ",int=" << i << ")" << endl;
        assert(i < INTS);
        msip(cpu) = 1;
    }

    static void ipi_eoi(Interrupt_Id i) { msip(CPU::id() + CPU_OFFSET) = 0; }

private:
    static void dispatch();

    // Logical handlers
    static void syscall(Interrupt_Id i);
    static void int_not(Interrupt_Id i);
    static void exception(Interrupt_Id i);

    // Physical handler
    static void entry() __attribute((naked, aligned(4)));

    static void init();

private:
    static Interrupt_Handler _int_vector[INTS];
};

__END_SYS

#endif
