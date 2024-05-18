// EPOS SiFive-U (RISC-V) SETUP

// If multitasking is enabled, configure the machine in supervisor mode and activate paging. Otherwise, keep the machine in machine mode.

#define __setup__

#include <architecture.h>
#include <boot_synchronizer.h>
#include <machine.h>
#include <utility/elf.h>
#include <utility/string.h>

extern "C" {
    void _start();

    // SETUP entry point is in .init (and not in .text), so it will be linked first and will be the first function after the ELF header in the image
    void _entry() __attribute__ ((used, naked, section(".init")));
    void _int_m2s() __attribute((naked, aligned(4)));
    void _setup();

    // LD eliminates this variable while performing garbage collection, that's why the used attribute.
    char __boot_time_system_info[sizeof(EPOS::S::System_Info)] __attribute__ ((used)) = "<System_Info placeholder>"; // actual System_Info will be added by mkbi!
}

__BEGIN_SYS

extern OStream kout, kerr;

class SV32_MMU;
class SV39_MMU;

class Setup
{
private:
    // Physical memory map
    static const unsigned long RAM_BASE         = Memory_Map::RAM_BASE;
    static const unsigned long RAM_TOP          = Memory_Map::RAM_TOP;
    static const unsigned long MIO_BASE         = Memory_Map::MIO_BASE;
    static const unsigned long MIO_TOP          = Memory_Map::MIO_TOP;
    static const unsigned long FREE_BASE        = Memory_Map::FREE_BASE; // only used in LIBRARY mode
    static const unsigned long FREE_TOP         = Memory_Map::FREE_TOP;  // only used in LIBRARY mode
    static const unsigned long SETUP            = Memory_Map::SETUP;
    static const unsigned long BOOT_STACK       = Memory_Map::BOOT_STACK;
    static const unsigned long INT_M2S          = Memory_Map::INT_M2S;
    static const unsigned long FLAT_MEM_MAP     = Memory_Map::FLAT_MEM_MAP;

    // Architecture Imports
    typedef CPU::Reg Reg;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;
    typedef IF<Traits<CPU>::WORD_SIZE == 32, SV32_MMU, SV39_MMU>::Result MMU; // architecture.h will use No_MMU if multitasking is disable, but we need the correct MMU for the Flat Memory Model.
    typedef MMU::Page Page;
    typedef MMU::Page_Flags Flags;
    typedef MMU::Page_Table Page_Table;
    typedef MMU::Page_Directory Page_Directory;
    typedef MMU::PT_Entry PT_Entry;
    typedef MMU::PD_Entry PD_Entry;
    typedef MMU::Chunk Chunk;
    typedef MMU::Directory Directory;

public:
    Setup();

private:
    void say_hi();

    void setup_flat_paging();
    void setup_m2s();
    void enable_paging();

    void call_next();

private:
    System_Info * si;
    MMU mmu;
};


Setup::Setup()
{
    si = reinterpret_cast<System_Info *>(&__boot_time_system_info);

    if(Boot_Synchronizer::acquire_single_core_section()) {
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

        // Memory paging is implemented only for the supervisor mode
        if (Traits<Machine>::supervisor) {
            // Configure a flat memory model for the single task in the system
            setup_flat_paging();

            // Relocate the machine to supervisor interrupt forwarder
            setup_m2s();

            // Enable paging
            enable_paging();
        }
    }

    // SETUP ends here, so let's transfer control to the next stage (INIT or APP)
    call_next();
}


void Setup::setup_flat_paging()
{
    db<Setup>(TRC) << "Setup::setup_flat_paging()" << endl;

    // Single-level mapping, 2 MB pages with SV32 and 1 GB pages with SV39
    static const unsigned long PD_ENTRIES = (Math::max(RAM_TOP, MIO_TOP) - Math::min(RAM_BASE, MIO_BASE) + sizeof(MMU::Huge_Page) - 1) / sizeof(MMU::Huge_Page);

    Page_Directory * pd = reinterpret_cast<Page_Directory *>(FLAT_MEM_MAP);
    Phy_Addr page = Math::min(RAM_BASE, MIO_BASE);
    for(unsigned int i = 0; i < PD_ENTRIES; i++, page += sizeof(MMU::Huge_Page))
        (*pd)[i] = (page >> 2) | MMU::Page_Flags::FLAT_MEM_PD;

    db<Setup>(INF) << "PD[" << pd << "]=" << *pd << endl;

    // Free chunks (passed to MMU::init())
    si->pmm.free1_base = MMU::align_page(FREE_BASE);
    si->pmm.free1_top = MMU::align_page(FREE_TOP);
}


void Setup::say_hi()
{
    db<Setup>(TRC) << "Setup::say_hi()" << endl;
    db<Setup>(INF) << "System_Info=" << *si << endl;

    kout << "\n*** This is EPOS!\n" << endl;
    kout << "Setting up this machine as follows: " << endl;
    kout << "  Mode:         " << ((Traits<Build>::SMOD == Traits<Build>::LIBRARY) ? "library" : (Traits<Build>::SMOD == Traits<Build>::BUILTIN) ? "built-in" : "kernel") << endl;
    kout << "  Processor:    " << Traits<Machine>::CPUS << " x RV" << Traits<CPU>::WORD_SIZE << " at " << Traits<CPU>::CLOCK / 1000000 << " MHz (BUS clock = " << Traits<Machine>::HFCLK / 1000000 << " MHz)" << endl;
    kout << "  Machine:      SiFive-U" << endl;
    kout << "  Memory:       " << (RAM_TOP + 1 - RAM_BASE) / 1024 << " KB [" << reinterpret_cast<void *>(RAM_BASE) << ":" << reinterpret_cast<void *>(RAM_TOP) << "]" << endl;
    kout << "  User memory:  " << (FREE_TOP - FREE_BASE) / 1024 << " KB [" << reinterpret_cast<void *>(FREE_BASE) << ":" << reinterpret_cast<void *>(FREE_TOP) << "]" << endl;
    kout << "  I/O space:    " << (MIO_TOP + 1 - MIO_BASE) / 1024 << " KB [" << reinterpret_cast<void *>(MIO_BASE) << ":" << reinterpret_cast<void *>(MIO_TOP) << "]" << endl;
    kout << "  Node Id:      ";
    if(si->bm.node_id != -1)
        kout << si->bm.node_id << endl;
    else
        kout << "will get from the network!" << endl;
    kout << "  Position:     ";
    if(si->bm.space_x != -1)
        kout << "(" << si->bm.space_x << "," << si->bm.space_y << "," << si->bm.space_z << ")" << endl;
    else
        kout << "will get from the network!" << endl;
    if(si->bm.extras_offset != -1UL)
        kout << "  Extras:       " << si->lm.app_extra_size << " bytes" << endl;

    kout << endl;
}


void Setup::setup_m2s()
{
    db<Setup>(TRC) << "Setup::setup_m2s()" << endl;

    memcpy(reinterpret_cast<void *>(INT_M2S), reinterpret_cast<void *>(&_int_m2s), sizeof(Page));
}


void Setup::enable_paging()
{
    db<Setup>(TRC) << "Setup::enable_paging()" << endl;
    if(Traits<Setup>::hysterically_debugged) {
        db<Setup>(INF) << "Setup::pc=" << CPU::pc() << endl;
        db<Setup>(INF) << "Setup::sp=" << CPU::sp() << endl;
    }

    // Set SATP and enable paging
    MMU::pd(FLAT_MEM_MAP);

    // Flush TLB to ensure we've got the right memory organization
    MMU::flush_tlb();

    if(Traits<Setup>::hysterically_debugged) {
        db<Setup>(INF) << "Setup::pc=" << CPU::pc() << endl;
        db<Setup>(INF) << "Setup::sp=" << CPU::sp() << endl;
    }
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

void _entry() // machine mode
{
    typedef IF<Traits<CPU>::WORD_SIZE == 32, SV32_MMU, SV39_MMU>::Result MMU; // architecture.h will use No_MMU if multitasking is disable, but we need the correct MMU for the Flat Memory Model.

    if(CPU::mhartid() == 0)                             // SiFive-U has 2 cores, but core 0 (an E51) does not feature an MMU, so we halt it and let core 1 (an U54) run in a single-core configuration
        CPU::halt();

    CPU::tp(CPU::mhartid() - 1);                        // tp will be CPU::id(); we won't count core 0, which is an heterogeneous E51
    CPU::mstatusc(CPU::MIE);                            // disable interrupts (they will be reenabled at Init_End)
    CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE * CPU::mhartid() - sizeof(long)); // set the stack pointer, thus creating a stack for SETUP for each core

    if(Boot_Synchronizer::acquire_single_core_section()) {
        Machine::clear_bss();
    }

    CPU::smp_barrier();

    if (Traits<Machine>::supervisor) {
        // If we are using the machine mode, we won't need to setup int_m2s() and also won't need to delegate
        // interruptions. The interruptions will also not be enabled here, since they will be enabled after
        // the interruption handler is set up.

        CPU::mtvec(CPU::INT_DIRECT, Memory_Map::INT_M2S);   // setup a machine mode interrupt handler to forward timer interrupts (which cannot be delegated via mideleg)
        CPU::mideleg(CPU::SSI | CPU::STI | CPU::SEI);       // delegate supervisor interrupts to supervisor mode
        CPU::medeleg(0xf1ff);                               // delegate all exceptions to supervisor mode but ecalls
        CPU::mie(CPU::MSI | CPU::MTI | CPU::MEI);           // enable interrupt generation by at machine level before going into supervisor mode
        CLINT::mtimecmp(-1ULL);                             // configure MTIMECMP so it won't trigger a timer interrupt before we can setup_m2s()
        CPU::mstatus(CPU::MPP_S | CPU::MPIE | CPU::MXR);    // prepare jump into supervisor mode at MRET with interrupts enabled at machine level
        CPU::mstatusc(CPU::SIE);                            // disable interrupts (they will be reenabled at Init_End)
        CPU::sstatuss(CPU::SUM);                            // allows User Memory access in supervisor mode
    } else {
        CPU::mstatus(CPU::MPP_M);  // Stays in machine mode (CPU::MPIE is not set this time because we didn't enable interruptions)
    }

    CPU::pmpcfg0(0b11111); 				// configure PMP region 0 as (L=unlocked [0], [00], A = NAPOT [11], X [1], W [1], R [1])
    CPU::pmpaddr0((1ULL << MMU::LA_BITS) - 1);          // comprising the whole memory space

    CPU::mepc(CPU::Reg(&_setup));                       // entry = _setup

    // If the supervisor mode is enabled, it will enter supervisor mode at setup (mepc) with interrupts enabled (mstatus.mpie = true).
    // otherwise, it will stay in machine mode and will run the setup with interrupts disabled.
    CPU::mret();
}

void _setup()
{
    kerr << endl;
    kout << endl;

    Setup setup;
}

// RISC-V's CLINT triggers interrupt 7 (MTI) whenever MTIME == MTIMECMP and there is no way to instruct it to trigger interrupt 9 (STI). So, even if we delegate all interrupts with MIDELEG, MTI doesn't turn into STI and MTI is visible in SIP. In other words, MTI must always be handled in machine mode, although the OS will run in supervisor mode.
// Therefore, an interrupt forwarder must be installed in machine mode to catch MTI and manually trigger STI. We use RAM_TOP for this, with the code at the beginning of the last page and per-core 256 bytes stacks at the end of the same page.
void _int_m2s()
{
    // Save context
    ASM("       csrw    mscratch, sp            \n");
if(Traits<CPU>::WORD_SIZE == 32) {
    ASM("       auipc    sp,      1             \n"     // SP = PC + 1 << 2 (INT_M2S + 4 + sizeof(Page))
        "       slli     gp, tp,  8             \n"
        "       sub      sp, sp, gp             \n"     // SP -= 256 * CPU::id()
        "       sw       a0,  -8(sp)            \n"
        "       sw       a1, -12(sp)            \n"
        "       sw       a2, -16(sp)            \n"
        "       sw       a3, -20(sp)            \n"
        "       sw       a4, -24(sp)            \n"
        "       sw       a5, -28(sp)            \n"
        "       sw       a6, -32(sp)            \n"
        "       sw       a7, -36(sp)            \n");
} else {
    ASM("       auipc    sp,      1             \n"     // SP = PC + 1 << 2 (INT_M2S + 4 + sizeof(Page))
        "       slli     gp, tp,  8             \n"
        "       sub      sp, sp, gp             \n"     // SP -= 256 * CPU::id()
        "       sd       a0, -12(sp)            \n"
        "       sd       a1, -20(sp)            \n"
        "       sd       a2, -28(sp)            \n"
        "       sd       a3, -36(sp)            \n"
        "       sd       a4, -44(sp)            \n"
        "       sd       a5, -52(sp)            \n");
}

    CPU::Reg id = CPU::mcause();

    if((id & CLINT::INT_MASK) == CLINT::IRQ_MAC_TIMER) {
        Timer::reset();                                 // MIP.MTI is a direct logic on (MTIME == MTIMECMP) and reseting the Timer (i.e. adjusting MTIMECMP) seems to be the only way to clear it
        CPU::mips(CPU::STI);                            // forward desired interrupts to supervisor mode
    } else if(id == CPU::EXC_ENVS) {
        CPU::mipc(CPU::STI);                            // STI was handled in supervisor mode, so clear the corresponding pending bit
        CPU::mepc(CPU::mepc() + 4);
    }

    // Restore context
if(Traits<CPU>::WORD_SIZE == 32) {
    ASM("       lw       a0,  -8(sp)            \n"
        "       lw       a1, -12(sp)            \n"
        "       lw       a2, -16(sp)            \n"
        "       lw       a3, -20(sp)            \n"
        "       lw       a4, -24(sp)            \n"
        "       lw       a5, -28(sp)            \n"
        "       lw       a6, -32(sp)            \n"
        "       lw       a7, -36(sp)            \n");
} else {
    ASM("       ld       a0, -12(sp)            \n"
        "       ld       a1, -20(sp)            \n"
        "       ld       a2, -28(sp)            \n"
        "       ld       a3, -36(sp)            \n"
        "       ld       a4, -44(sp)            \n"
        "       ld       a5, -52(sp)            \n");
}
    ASM("       csrr     sp, mscratch           \n"
        "       mret                            \n");
}

