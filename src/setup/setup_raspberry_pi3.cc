// EPOS Raspberry Pi3 (Cortex-A53) SETUP

#include <architecture.h>
#include <machine.h>
#include <system/memory_map.h>
#include <utility/elf.h>
#include <utility/string.h>

extern "C" {
    void _start();

    // SETUP entry point is the Vector Table and resides in the .init section (not in .text), so it will be linked first and will be the first function after the ELF header in the image.
    void _entry() __attribute__ ((used, naked, section(".init")));
    void _vector_table() __attribute__ ((naked));
    void _reset() __attribute__ ((naked)); // so it can be safely reached from the vector table
    void _setup(); // just to create a Setup object

    // LD eliminates this variable while performing garbage collection, that's why the used attribute.
    char __boot_time_system_info[sizeof(EPOS::S::System_Info)] __attribute__ ((used)) = "<System_Info placeholder>"; // actual System_Info will be added by mkbi!
}

__BEGIN_SYS

extern OStream kout, kerr;

class Setup
{
private:
    // Physical memory map
    static const unsigned long RAM_BASE         = Memory_Map::RAM_BASE;
    static const unsigned long RAM_TOP          = Memory_Map::RAM_TOP;
    static const unsigned long MIO_BASE         = Memory_Map::MIO_BASE;
    static const unsigned long MIO_TOP          = Memory_Map::MIO_TOP;
    static const unsigned long IMAGE            = Memory_Map::IMAGE;
    static const unsigned long SETUP            = Memory_Map::SETUP;
    static const unsigned long FLAT_PAGE_TABLE  = Memory_Map::FLAT_PAGE_TABLE;
    static const unsigned long FREE_BASE        = Memory_Map::FREE_BASE;
    static const unsigned long FREE_TOP         = Memory_Map::FREE_TOP;

    // Architecture Imports
    typedef CPU::Reg Reg;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;

public:
    Setup();

private:
    void say_hi();
    void setup_flat_paging();
    void enable_paging();
    void call_next();

private:
    System_Info * si;
};

Setup::Setup()
{
    si = reinterpret_cast<System_Info *>(&__boot_time_system_info);

    // SETUP doesn't handle global constructors, so we need to manually initialize any object with a non-empty default constructor
    new (&kout) OStream;
    new (&kerr) OStream;
    Display::init();
    kout << endl;
    kerr << endl;

    db<Setup>(TRC) << "Setup(si=" << reinterpret_cast<void *>(si) << ",sp=" << CPU::sp() << ")" << endl;
    db<Setup>(INF) << "Setup:si=" << *si << endl;

    // Print basic facts about this EPOS instance
    say_hi();

    // Configure a flat memory model for the single task in the system
    setup_flat_paging();

    // Enable paging
    enable_paging();

    // SETUP ends here, so let's transfer control to the next stage (INIT or APP)
    call_next();
}


void Setup::setup_flat_paging()
{
    db<Setup>(TRC) << "Setup::setup_flat_paging()" << endl;

#ifdef __armv7__
    
    CPU::Reg * pt = reinterpret_cast<CPU::Reg *>(FLAT_PAGE_TABLE);
    for(CPU::Reg i = MMU_Common<12,0,20>::pdi(RAM_BASE); i < MMU_Common<12,0,20>::pdi(RAM_BASE) + MMU_Common<12,0,20>::pages(RAM_TOP - RAM_BASE); i++)
        pt[(i & 0x3ff)] = (i << 20) | ARMv7_MMU::Section_Flags::FLAT_MEMORY_MEM; // the mask prevents GCC warning about integer overflow and aggressive loop optimization
    pt[MMU_Common<12,0,20>::pdi(MIO_BASE)] = (MMU_Common<12,0,20>::pdi(MIO_BASE) << 20) | ARMv7_MMU::Section_Flags::FLAT_MEMORY_DEV;
    for(CPU::Reg i = MMU_Common<12,0,20>::pdi(MIO_BASE) + 1; i < MMU_Common<12,0,20>::pdi(MIO_BASE) + MMU_Common<12,0,20>::pages(MIO_TOP - MIO_BASE); i++)
        pt[i] = (i << 20) | ARMv7_MMU::Section_Flags::FLAT_MEMORY_PER;

#else

    // Single-level mapping, 32 MB blocks
    static const unsigned long BLOCK_SIZE       = 32 * 1024 * 1024; // 32 MB
    static const unsigned long PD_ENTRIES       = 128; //(Math::max(RAM_TOP, MIO_TOP) - Math::min(RAM_BASE, MIO_BASE)) / 32 * 1024 * 1024; // 128 for 4GB

    CPU::Reg mem = RAM_BASE;
    CPU::Reg * pd = reinterpret_cast<CPU::Reg *>(FLAT_PAGE_TABLE);

    for(unsigned int i = 0; i < PD_ENTRIES; i++, mem += BLOCK_SIZE)
        pd[i] = reinterpret_cast<CPU::Reg>(mem) | ARMv8_MMU::Page_Flags::FLAT_MEM_BLOCK;

//    // Two-level mapping, 16KB granule
//    // To used this code, remember to adjust FLAT_PAGE_TABLE to account for all the associated page tables (e.g. (RAM_TOP - (PD_ETRIES + 1) * 16 * 1024) & ~(0x3fff)
//    static const unsigned long PAGE_SIZE        = 16 * 1024; // 16 KB
//    static const unsigned long PD_ENTRIES       = 128; //(Math::max(RAM_TOP, MIO_TOP) - Math::min(RAM_BASE, MIO_BASE)) / 32 * 1024 * 1024; // 128 for 4GB
//    static const unsigned long PT_ENTRIES       = PAGE_SIZE / sizeof(long); // 2 K entries
//    static const unsigned long PD_MASK          = ((PAGE_SIZE - 1) | (0xfUL << 52));
//
//    CPU::Reg mem = RAM_BASE;
//    CPU::Reg * pts = reinterpret_cast<CPU::Reg *>(FLAT_PAGE_TABLE);
//
//    CPU::Reg * pd = reinterpret_cast<CPU::Reg *>(pts); // PD is the first page in pts_mem
//    pts += PAGE_SIZE;
//    for(unsigned int i = 0; i < PD_ENTRIES; i++, pts += PT_ENTRIES)
//        pd[i] = reinterpret_cast<CPU::Reg>(pts) | ARMv8_MMU::Page_Flags::PAGE_DESCRIPTOR | ARMv8_MMU::Page_Flags::OUTER_SHAREABLE | ARMv8_MMU::Page_Flags::ACCESS | ARMv8_MMU::Page_Flags::EL1_XN | ARMv8_MMU::Page_Flags::XN;
//
//    for(unsigned int i = 0; i < PD_ENTRIES; i++) {
//        CPU::Reg * pt = reinterpret_cast<CPU::Reg *>(pd[i] & ~PD_MASK);
//        db<Setup>(TRC) << "PT[" << i << "]: " << hex << pt << endl;
//        for(unsigned int j = 0; j < PT_ENTRIES; j++, mem += PAGE_SIZE)
//            pt[j] = reinterpret_cast<CPU::Reg>(mem) | ARMv8_MMU::Page_Flags::PAGE_DESCRIPTOR | ARMv8_MMU::Page_Flags::INNER_SHAREABLE | ARMv8_MMU::Page_Flags::SEL_MAIR_ATTR0 | ARMv8_MMU::Page_Flags::ACCESS;
//    }

    CPU::mair_el1((CPU::ATTR_DEVICE_nGnRnE) << 0 * CPU::ATTR_OFFSET | // first attribute_index
        (CPU::ATTR_NORMAL_WT) << 1 * CPU::ATTR_OFFSET | // second attribute index
        (CPU::ATTR_NORMAL_WB) << 2 * CPU::ATTR_OFFSET | // second attribute index
        (CPU::ATTR_NORMAL_NON_CACHE) << 3 * CPU::ATTR_OFFSET); // second attribute index

#endif
}


void Setup::say_hi()
{
    db<Setup>(TRC) << "Setup::say_hi()" << endl;
    db<Setup>(INF) << "System_Info=" << *si << endl;

    kout << "This is EPOS!\n" << endl;
    kout << "Setting up this machine as follows: " << endl;
    kout << "  Mode:         " << ((Traits<Build>::MODE == Traits<Build>::LIBRARY) ? "library" : (Traits<Build>::MODE == Traits<Build>::BUILTIN) ? "built-in" : "kernel") << endl;
    kout << "  Processor:    " << Traits<Machine>::CPUS << " x Cortex A53 (ARMv" << ((Traits<CPU>::WORD_SIZE == 32) ? "7" : "8") << "-A) at " << Traits<CPU>::CLOCK / 1000000 << " MHz (BUS clock = " << Traits<CPU>::CLOCK / 1000000 << " MHz)" << endl;
    kout << "  Machine:      Raspberry Pi3" << endl;
    kout << "  Memory:       " << (RAM_TOP + 1 - RAM_BASE) / 1024 << " KB [" << reinterpret_cast<void *>(RAM_BASE) << ":" << reinterpret_cast<void *>(RAM_TOP) << "]" << endl;
    kout << "  User memory:  " << (FREE_TOP - FREE_BASE) / 1024 << " KB [" << reinterpret_cast<void *>(FREE_BASE) << ":" << reinterpret_cast<void *>(FREE_TOP) << "]" << endl;
    kout << "  I/O space:    " << (MIO_TOP + 1 - MIO_BASE) / 1024 << " KB [" << reinterpret_cast<void *>(MIO_BASE) << ":" << reinterpret_cast<void *>(MIO_TOP) << "]" << endl;
    kout << "  Node Id:      ";
    if(si->bm.node_id != -1)
        kout << si->bm.node_id << " (" << Traits<Build>::NODES << ")" << endl;
    else
        kout << "will get from the network!" << endl;
    kout << "  Position:     ";
    if(si->bm.space_x != -1)
        kout << "(" << si->bm.space_x << "," << si->bm.space_y << "," << si->bm.space_z << ")" << endl;
    else
        kout << "will get from the network!" << endl;
    if(si->lm.has_stp)
        kout << "  Setup:        " << si->lm.stp_code_size + si->lm.stp_data_size << " bytes" << endl;
    if(si->lm.has_ini)
        kout << "  Init:         " << si->lm.ini_code_size + si->lm.ini_data_size << " bytes" << endl;
    if(si->lm.has_sys)
        kout << "  OS code:      " << si->lm.sys_code_size << " bytes" << "\tdata: " << si->lm.sys_data_size << " bytes" << "   stack: " << si->lm.sys_stack_size << " bytes" << endl;
    if(si->lm.has_app)
        kout << "  APP code:     " << si->lm.app_code_size << " bytes" << "\tdata: " << si->lm.app_data_size << " bytes" << endl;
    if(si->lm.has_ext)
        kout << "  Extras:       " << si->lm.app_extra_size << " bytes" << endl;

    kout << endl;
}


void Setup::enable_paging()
{
    db<Setup>(TRC) << "Setup::enable_paging()" << endl;
    if(Traits<Setup>::hysterically_debugged) {
        db<Setup>(INF) << "Setup::pc=" << CPU::pc() << endl;
        db<Setup>(INF) << "Setup::sp=" << CPU::sp() << endl;
    }

#ifdef __armv7__

    // MNG_DOMAIN for no page permission verification, CLI_DOMAIN otherwise
    CPU::dacr(CPU::MNG_DOMAIN);

    CPU::dsb();
    CPU::isb();

    // Clear TTBCR for the system to use ttbr0 instead of 1
    CPU::ttbcr(0);
    // Set ttbr0 with base address
    CPU::ttbr0(FLAT_PAGE_TABLE);

    // Enable MMU through SCTLR and ACTLR
    CPU::actlr(CPU::actlr() | CPU::SMP); // Set SMP bit
    CPU::sctlr((CPU::sctlr() | CPU::DCACHE | CPU::ICACHE | CPU::MMU_ENABLE) & ~(CPU::AFE));

    CPU::dsb();
    CPU::isb();

    // MMU now enabled - Virtual address system now active
    // Branch Prediction Enable
    CPU::sctlr(CPU::sctlr() | CPU::BRANCH_PRED);

    // Flush TLB to ensure we've got the right memory organization
    MMU::flush_tlb();

    // Adjust pointers that will still be used to their logical addresses
    Display::init(); // adjust the pointers in Display by calling init

    if(Traits<Setup>::hysterically_debugged) {
        db<Setup>(INF) << "pc=" << CPU::pc() << endl;
        db<Setup>(INF) << "sp=" << CPU::sp() << endl;
    }

#else 

    // Configure paging with two levels and 16KB pages via TTBRC
    CPU::ttbcr(CPU::TTBR1_DISABLE | CPU::TTBR0_WALK_INNER_SHAREABLE | CPU::TTBR0_WALK_OUTER_WB_WA | CPU::TTBR0_WALK_INNER_WB_WA | CPU::TTBR0_TG0_16KB | CPU::TTBR0_SIZE_4GB);
    CPU::isb();

    // Tell the MMU where our translation tables are
    db<Setup>(INF) << "SYS_PD=" << reinterpret_cast<void *>(FLAT_PAGE_TABLE) << endl;
    CPU::ttbr0(FLAT_PAGE_TABLE);

    CPU::dsb();
    CPU::isb();

    CPU::sctlr(CPU::sctlr() | CPU::MMU_ENABLE | CPU::DCACHE | CPU::ICACHE);
    CPU::isb();

    Display::init();

#endif
}


void Setup::call_next()
{
    db<Setup>(INF) << "SETUP ends here!" << endl;

    // Call the next stage
    static_cast<void (*)()>(_start)();

    // SETUP is now part of the free memory and this point should never be reached, but, just in case ... :-)
    db<Setup>(ERR) << "OS failed to init!" << endl;
}

__END_SYS

using namespace EPOS::S;

#ifdef __armv7__

void _entry()
{
    // Interrupt Vector Table
    // We use and indirection table for the ldr instructions because the offset can be to far from the PC to be encoded
    ASM("               ldr pc, .reset                                          \t\n\
                        ldr pc, .ui                                             \t\n\
                        ldr pc, .si                                             \t\n\
                        ldr pc, .pa                                             \t\n\
                        ldr pc, .da                                             \t\n\
                        ldr pc, .reserved                                       \t\n\
                        ldr pc, .irq                                            \t\n\
                        ldr pc, .fiq                                            \t\n\
                                                                                \t\n\
                        .balign 32                                              \t\n\
        .reset:         .word _reset                                            \t\n\
        .ui:            .word 0x0                                               \t\n\
        .si:            .word 0x0                                               \t\n\
        .pa:            .word 0x0                                               \t\n\
        .da:            .word 0x0                                               \t\n\
        .reserved:      .word 0x0                                               \t\n\
        .irq:           .word 0x0                                               \t\n\
        .fiq:           .word 0x0                                               \t");
}

void _reset()
{
    CPU::int_disable(); // interrupts will be re-enabled at init_end

    if(CPU::id() != 0) // We want secondary cores to be held here
        CPU::halt();

    // QEMU get us here in SVC mode with interrupt disabled, but the real Raspberry Pi3 starts in hypervisor mode, so we must switch to SVC mode
    if(!Traits<Machine>::emulated) {
        CPU::Reg cpsr = CPU::psr();
        cpsr &= ~CPU::FLAG_M;           // clear mode bits
        cpsr |= CPU::MODE_SVC;          // set supervisor flag
        CPU::psr(cpsr);                 // enter supervisor mode
        CPU::Reg address = CPU::ra();
        CPU::elr_hyp(address);
        CPU::tmp_to_cpsr();
    }

    // Configure a stack for SVC mode, which will be used until the first Thread is created
    CPU::mode(CPU::MODE_SVC); // enter SVC mode (with interrupts disabled)
    CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE * (CPU::id() + 1) - sizeof(long));

    // After a reset, we copy the vector table to 0x0000 to get a cleaner memory map (it is originally at 0x8000)
    // An alternative would be to set vbar address via mrc p15, 0, r1, c12, c0, 0
    CPU::r0(reinterpret_cast<CPU::Reg>(&_entry)); // load r0 with the source pointer
    CPU::r1(Memory_Map::VECTOR_TABLE); // load r1 with the destination pointer

    // Copy the first 32 bytes
    CPU::ldmia(); // load multiple registers from the memory pointed by r0 and auto-increment it accordingly
    CPU::stmia(); // store multiple registers to the memory pointed by r1 and auto-increment it accordingly

    // Repeat to copy the subsequent 32 bytes
    CPU::ldmia();
    CPU::stmia();

    // Clear the BSS (SETUP was linked to CRT0, but entry point didn't go through BSS clear)
    Machine::clear_bss();

    // Set VBAR to point to the relocated the vector table
    CPU::vbar(Memory_Map::VECTOR_TABLE);

    CPU::flush_caches();
    CPU::flush_branch_predictors();
    CPU::flush_tlb();
    CPU::actlr(CPU::actlr() | CPU::DCACHE_PREFETCH); // enable D-side prefetch

    CPU::dsb();
    CPU::isb();

    _setup();
}

#else

void _entry()
{
    // Manual D13.2.102 explains the cold start of an ARMv8, with RVBAR_EL3 defined by the SoC's manufaturer
    // Broadcom uses 0 and the GPU initialized that memory region with code that end up jumping to 0x80000 (i.e. _entry)

    // Set SP to avoid early faults
    CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE * (CPU::id() + 1)); // set stack

    ASM("b _reset");
}

void _vector_table()
{
    // Manual D1.10.2, page D1-1430 shows four configurations in terms of the exception level an exception can came from:
    // current level with SP_EL0, current level with SP_ELx, lower level AArch64, and lower level AArch32.
    // Additionally, there are four exception types: synchronous exception, IRQ, FIQ, and SError.
    // Therefore, each exception level maps the four exception type handlers aligned by 128 bytes (enough room to write simple handlers).

    // Our strategy is to forward all interrupts to _int_entry via .ic_entry. Since x30 (LR) contains the return address and will be adjusted by the "blr" instruction,
    // we save x29 to reach _int_entry. The value of .ic_entry will be set by IC::init(). To match ARMv7, we also disable interrupts (which are reenabled at dispatch).
    ASM("// Current EL with SP0                                                 \t\n\
                        .balign 128                                             \t\n\
        .sync_curr_sp0: str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .irq_curr_sp0:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .fiq_curr_sp0:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .error_curr_sp0:str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
        // Current EL with SPx                                                  \t\n\
                        .balign 128                                             \t\n\
        .sync_curr_spx: str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .irq_curr_spx:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .fiq_curr_spx:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .error_curr_spx:str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
        // Lower EL using AArch64                                               \t\n\
                        .balign 128                                             \t\n\
        .sync_lower64:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .irq_lower64:   str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .fiq_lower64:   str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .error_lower64: str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        // Lower EL using AArch32                               \t\n\
                        .balign 128                                             \t\n\
        .sync_lower32:  str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .irq_lower32:   str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .fiq_lower32:   str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .error_lower32: str x30, [sp,#-8]!                                      \t\n\
                        str x29, [sp,#-8]!                                      \t\n\
                        ldr x29, .ic_entry                                      \t\n\
                        blr x29                                                 \t\n\
                        ldr x29, [sp], #8                                       \t\n\
                        ldr x30, [sp], #8                                       \t\n\
                        eret                                                    \t\n\
                                                                                \t\n\
                        .balign 128                                             \t\n\
        .ic_entry:      .dword 0x0             // set by IC::init()             \t");
}

void _reset()
{
    CPU::int_disable(); // interrupts will be re-enabled at init_end

    if(CPU::id() != 0) // We want secondary cores to be held here
        CPU::halt();

    // Relocated the vector table, which has 4 entries for each of the 4 scenarios, all 128 bytes aligned, plus an 8 bytes pointer, totaling 2056 bytes
    CPU::Reg * src = reinterpret_cast<CPU::Reg *>(&_vector_table);
    CPU::Reg * dst = reinterpret_cast<CPU::Reg *>(Memory_Map::VECTOR_TABLE);
    for(int i = 0; i < (2056 / 8); i++)
        dst[i] = src[i];

    // Clear the BSS (SETUP was linked to CRT0, but entry point didn't go through BSS clear)
    Machine::clear_bss();

    // Set EL1 VBAR to the relocated vector table
    CPU::vbar_el1(static_cast<CPU::Phy_Addr>(Memory_Map::VECTOR_TABLE));

    // Activate aarch64
    CPU::hcr(CPU::EL1_AARCH64_EN | CPU::SWIO_HARDWIRED);

    // We start at EL2, but must set EL1 SP for a smooth transition, including further exception/interrupt handling
    CPU::spsr_el2(CPU::FLAG_D | CPU::FLAG_A | CPU::FLAG_I | CPU::FLAG_F | CPU::FLAG_EL1 | CPU::FLAG_SP_ELn);
    CPU::Reg el1_addr = CPU::pc();
    el1_addr += 16; // previous instruction, this instruction, and the next one;
    CPU::elr_el2(el1_addr);
    CPU::eret();
    CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE * (CPU::id() + 1)); // set stack

    _setup();
}

#endif

void _setup()
{
    Setup setup;
}
