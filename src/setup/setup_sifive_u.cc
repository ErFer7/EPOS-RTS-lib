// EPOS SiFive-U (RISC-V) SETUP

// If multitasking is enabled, configure the machine in supervisor mode and activate paging. Otherwise, keep the machine in machine mode.

#define __setup__

#include <architecture.h>
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
    static const unsigned long IMAGE            = Memory_Map::IMAGE;
    static const unsigned long SETUP            = Memory_Map::SETUP;
    static const unsigned long BOOT_STACK       = Memory_Map::BOOT_STACK;
    static const unsigned long INT_M2S          = Memory_Map::INT_M2S;
    static const unsigned long FLAT_MEM_MAP     = Memory_Map::FLAT_MEM_MAP;

    // Logical memory map
    static const unsigned long APP_LOW          = Memory_Map::APP_LOW;
    static const unsigned long APP_HIGH         = Memory_Map::APP_HIGH;
    static const unsigned long INIT             = Memory_Map::INIT;
    static const unsigned long PHY_MEM          = Memory_Map::PHY_MEM;
    static const unsigned long IO               = Memory_Map::IO;
    static const unsigned long SYS              = Memory_Map::SYS;
    static const unsigned long SYS_INFO         = Memory_Map::SYS_INFO;
    static const unsigned long SYS_PT           = Memory_Map::SYS_PT;
    static const unsigned long SYS_PD           = Memory_Map::SYS_PD;
    static const unsigned long SYS_CODE         = Memory_Map::SYS_CODE;
    static const unsigned long SYS_DATA         = Memory_Map::SYS_DATA;
    static const unsigned long SYS_STACK        = Memory_Map::SYS_STACK;
    static const unsigned long SYS_HEAP         = Memory_Map::SYS_HEAP;
    static const unsigned long APP_CODE         = Memory_Map::APP_CODE;
    static const unsigned long APP_DATA         = Memory_Map::APP_DATA;

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
    void build_lm();
    void build_pmm();

    void say_hi();

    void setup_m2s();
    void setup_sys_pt();
    void setup_app_pt();
    void setup_sys_pd();
    void setup_flat_paging();
    void enable_paging();

    void load_parts();
    void adjust_perms();
    void call_next();

private:
    char * bi;
    System_Info * si;
    MMU mmu;
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

    // Relocate the machine to supervisor interrupt forwarder
    setup_m2s();

    // Enable paging
    enable_paging();

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


void Setup::build_lm()
{
    db<Setup>(TRC) << "Setup::build_lm()" << endl;

    // Get boot image structure
    si->lm.has_stp = (si->bm.setup_offset != -1ul);
    si->lm.has_ini = (si->bm.init_offset != -1ul);
    si->lm.has_sys = (si->bm.system_offset != -1ul);
    si->lm.has_app = (si->bm.application_offset != -1ul);
    si->lm.has_ext = (si->bm.extras_offset != -1ul);

    // Check SETUP integrity and get the size of its segments
    if(si->lm.has_stp) {
        ELF * stp_elf = reinterpret_cast<ELF *>(&bi[si->bm.setup_offset]);
        if(!stp_elf->valid())
            db<Setup>(ERR) << "SETUP ELF image is corrupted!" << endl;
        stp_elf->scan(reinterpret_cast<ELF::Loadable *>(&si->lm.stp_entry), SETUP, SETUP + 64 * 1024, 0, 0);
    }

    // Check INIT integrity and get the size of its segments
    if(si->lm.has_ini) {
        ELF * ini_elf = reinterpret_cast<ELF *>(&bi[si->bm.init_offset]);
        if(!ini_elf->valid())
            db<Setup>(ERR) << "INIT ELF image is corrupted!" << endl;

        ini_elf->scan(reinterpret_cast<ELF::Loadable *>(&si->lm.ini_entry), INIT, INIT + 64 * 1024, 0, 0);
    }

    // Check SYSTEM integrity and get the size of its segments
    si->lm.sys_stack = SYS_STACK;
    si->lm.sys_stack_size = Traits<System>::STACK_SIZE * Traits<Build>::CPUS;
    if(si->lm.has_sys) {
        ELF * sys_elf = reinterpret_cast<ELF *>(&bi[si->bm.system_offset]);
        if(!sys_elf->valid())
            db<Setup>(ERR) << "OS ELF image is corrupted!" << endl;

        sys_elf->scan(reinterpret_cast<ELF::Loadable *>(&si->lm.sys_entry), SYS_CODE, SYS_DATA - 1, SYS_DATA, SYS_STACK - 1);

        if(si->lm.sys_code != SYS_CODE)
            db<Setup>(ERR) << "OS code segment address (" << reinterpret_cast<void *>(si->lm.sys_code) << ") does not match the machine's memory map (" << reinterpret_cast<void *>(SYS_CODE) << ")!" << endl;
        if(si->lm.sys_code + si->lm.sys_code_size > si->lm.sys_data)
            db<Setup>(ERR) << "OS code segment is too large!" << endl;
        if(si->lm.sys_data != SYS_DATA)
            db<Setup>(ERR) << "OS data segment address (" << reinterpret_cast<void *>(si->lm.sys_data) << ") does not match the machine's memory map (" << reinterpret_cast<void *>(SYS_DATA) << ")!" << endl;
        if(si->lm.sys_data + si->lm.sys_data_size > si->lm.sys_stack)
            db<Setup>(ERR) << "OS data segment is too large!" << endl;
        if(MMU::pts(MMU::pages(si->lm.sys_stack_size)) > 1)
            db<Setup>(ERR) << "OS stack segment is too large!" << endl;
    }

    // Check APPLICATION integrity and get the size of its segments
    if(si->lm.has_app) {
        ELF * app_elf = reinterpret_cast<ELF *>(&bi[si->bm.application_offset]);
        if(!app_elf->valid())
            db<Setup>(ERR) << "APP ELF image is corrupted!" << endl;

        app_elf->scan(reinterpret_cast<ELF::Loadable *>(&si->lm.app_entry), APP_CODE, APP_DATA - 1, APP_DATA, APP_HIGH);

        if(si->lm.app_code != MMU::align_segment(si->lm.app_code))
            db<Setup>(ERR) << "Unaligned APP code segment:" << hex << si->lm.app_code << endl;
        if(si->lm.app_data_size == 0) {
            db<Setup>(WRN) << "APP ELF image has no data segment!" << endl;
            si->lm.app_data = MMU::align_page(APP_DATA);
        }
        if(Traits<System>::multiheap) { // Application heap in data segment
            si->lm.app_data_size = MMU::align_page(si->lm.app_data_size);
            si->lm.app_stack = si->lm.app_data + si->lm.app_data_size;
            si->lm.app_data_size += MMU::align_page(Traits<Application>::STACK_SIZE);
            si->lm.app_heap = si->lm.app_data + si->lm.app_data_size;
            si->lm.app_data_size += MMU::align_page(Traits<Application>::HEAP_SIZE);
        }
        if(si->lm.has_ext) { // Check for EXTRA data in the boot image
            si->lm.app_extra = si->lm.app_data + si->lm.app_data_size;
            si->lm.app_extra_size = si->bm.img_size - si->bm.extras_offset;
            if(Traits<System>::multiheap)
                si->lm.app_extra_size = MMU::align_page(si->lm.app_extra_size);
            si->lm.app_data_size += si->lm.app_extra_size;
        } else {
            si->lm.app_extra = ~0U;
            si->lm.app_extra_size = 0;
        }
    }
}


void Setup::build_pmm()
{
    db<Setup>(TRC) << "Setup::build_pmm()" << endl;

    // Initialize a temporary MMU with the top half of the memory just for SETUP to allocate architectural data structures
    mmu.free(MMU::align_page(RAM_BASE + (RAM_TOP + 1 - RAM_BASE) / 2), MMU::pages((RAM_TOP + 1 - RAM_BASE) / 2));

    // Allocate (reserve) memory for all entities we have to setup.
    // We'll start at the highest address to make possible a memory model
    // on which the application's logical and physical address spaces match.

    // Machine to Supervisor Interrupt Forwarder (1 x sizeof(Page), not listed in the PMM)
    MMU::alloc();

    // Boot stack (already in use at Memory_Map::BOOT_STACK; listed in the PMM as a free region to be recovered at MMU::init())
    si->pmm.free2_base = MMU::alloc(MMU::pages(Traits<Machine>::STACK_SIZE) * Traits<Build>::CPUS);
    si->pmm.free2_top = si->pmm.free2_base + Traits<Machine>::STACK_SIZE * Traits<Build>::CPUS;

    // System Page Directory
    si->pmm.sys_pd = MMU::calloc();

    // System Page Table
    si->pmm.sys_pt = MMU::calloc(MMU::pts(MMU::pages(SYS_HEAP - SYS)));

    // Page tables to map the whole physical memory
    // = NP/NPTE_PT * sizeof(Page)
    //   NP = size of physical memory in pages
    //   NPTE_PT = number of page table entries per page table
    si->pmm.phy_mem_pt = MMU::calloc(MMU::pts(MMU::pages(si->bm.mem_top - si->bm.mem_base)));

    // Page tables to map the IO address space
    // = NP/NPTE_PT * sizeof(Page)
    //   NP = size of I/O address space in pages
    //   NPTE_PT = number of page table entries per page table
    si->pmm.io_pt = MMU::calloc(MMU::pts(MMU::pages(si->bm.mio_top - si->bm.mio_base)));

    // Page tables to map the first APPLICATION code segment
    si->pmm.app_code_pt = MMU::calloc(MMU::pts(MMU::pages(si->lm.app_code_size)));

    // Page tables to map the first APPLICATION data segment (which contains heap, stack and extra)
    si->pmm.app_data_pt = MMU::calloc(MMU::pts(MMU::pages(si->lm.app_data_size)));

    // System Info
    si->pmm.sys_info = (SYS_INFO != Traits<Machine>::NOT_USED) ? MMU::calloc() : Phy_Addr(false);

    // SYSTEM code segment
    si->pmm.sys_code = MMU::alloc(MMU::pages(si->lm.sys_code_size));

    // SYSTEM data segment
    si->pmm.sys_data = MMU::alloc(MMU::pages(si->lm.sys_data_size));

    // SYSTEM stack segment
    si->pmm.sys_stack = MMU::alloc(MMU::pages(si->lm.sys_stack_size));

    // The memory allocated so far will "disappear" from the system as we set usr_mem_top as follows:
    si->pmm.usr_mem_base = si->bm.mem_base;
    si->pmm.usr_mem_top = si->bm.mem_base + MMU::allocable() * sizeof(Page);

    // APPLICATION code segment
    si->pmm.app_code = MMU::alloc(MMU::pages(si->lm.app_code_size));

    // APPLICATION data segment (contains stack, heap and extra)
    si->pmm.app_data = MMU::alloc(MMU::pages(si->lm.app_data_size));

    // Free chunk 1 (passed to MMU::init) must be defined after setup_sys_pd, since attaching implicitly allocates attachers.
    // Free chunk 2 contains the boot stack.
    // Free chunk 3 is not used.
    si->pmm.free3_base = 0;
    si->pmm.free3_top = 0;

    // Test if we didn't overlap SETUP and the boot image
    if(si->pmm.usr_mem_top <= si->lm.stp_code + si->lm.stp_code_size + si->lm.stp_data_size)
        db<Setup>(ERR) << "SETUP would have been overwritten!" << endl;
}


void Setup::say_hi()
{
    db<Setup>(TRC) << "Setup::say_hi()" << endl;
    db<Setup>(INF) << "System_Info=" << *si << endl;

    kout << "This is EPOS!\n" << endl;
    kout << "Setting up this machine as follows: " << endl;
    kout << "  Mode:         " << ((Traits<Build>::SMOD == Traits<Build>::LIBRARY) ? "library" : (Traits<Build>::SMOD == Traits<Build>::BUILTIN) ? "built-in" : "kernel") << endl;
    kout << "  Processor:    " << Traits<Machine>::CPUS << " x RV" << Traits<CPU>::WORD_SIZE << " at " << Traits<CPU>::CLOCK / 1000000 << " MHz (BUS clock = " << Traits<Machine>::HFCLK / 1000000 << " MHz)" << endl;
    kout << "  Machine:      SiFive-U" << endl;
#ifdef __library__
    kout << "  Memory:       " << (RAM_TOP + 1 - RAM_BASE) / 1024 << " KB [" << reinterpret_cast<void *>(RAM_BASE) << ":" << reinterpret_cast<void *>(RAM_TOP) << "]" << endl;
    kout << "  User memory:  " << (FREE_TOP - FREE_BASE) / 1024 << " KB [" << reinterpret_cast<void *>(FREE_BASE) << ":" << reinterpret_cast<void *>(FREE_TOP) << "]" << endl;
    kout << "  I/O space:    " << (MIO_TOP + 1 - MIO_BASE) / 1024 << " KB [" << reinterpret_cast<void *>(MIO_BASE) << ":" << reinterpret_cast<void *>(MIO_TOP) << "]" << endl;
#else
    kout << "  Memory:       " << (si->bm.mem_top - si->bm.mem_base) / 1024 << " KB [" << reinterpret_cast<void *>(si->bm.mem_base) << ":" << reinterpret_cast<void *>(si->bm.mem_top) << "]" << endl;
    kout << "  User memory:  " << (si->pmm.usr_mem_top - si->pmm.usr_mem_base) / 1024 << " KB [" << reinterpret_cast<void *>(si->pmm.usr_mem_base) << ":" << reinterpret_cast<void *>(si->pmm.usr_mem_top) << "]" << endl;
    kout << "  I/O space:    " << (si->bm.mio_top - si->bm.mio_base) / 1024 << " KB [" << reinterpret_cast<void *>(si->bm.mio_base) << ":" << reinterpret_cast<void *>(si->bm.mio_top) << "]" << endl;
#endif
    kout << "  Node Id:      ";
    if(si->bm.node_id != -1)
        kout << si->bm.node_id << " (" << Traits<Build>::NETWORKING << ")" << endl;
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


void Setup::setup_sys_pt()
{
    db<Setup>(TRC) << "Setup::setup_sys_pt(pmm="
                   << "{si="      << reinterpret_cast<void *>(si->pmm.sys_info)
                   << ",pt="      << reinterpret_cast<void *>(si->pmm.sys_pt)
                   << ",pd="      << reinterpret_cast<void *>(si->pmm.sys_pd)
                   << ",sysc={b=" << reinterpret_cast<void *>(si->pmm.sys_code) << ",s=" << MMU::pages(si->lm.sys_code_size) << "}"
                   << ",sysd={b=" << reinterpret_cast<void *>(si->pmm.sys_data) << ",s=" << MMU::pages(si->lm.sys_data_size) << "}"
                   << ",syss={b=" << reinterpret_cast<void *>(si->pmm.sys_stack) << ",s=" << MMU::pages(si->lm.sys_stack_size) << "}"
                   << "})" << endl;

    // Get the physical address for the SYSTEM Page Table, which was allocated with calloc()
    Page_Table * sys_pt = reinterpret_cast<Page_Table *>(si->pmm.sys_pt);

    // System Info
    sys_pt->remap(si->pmm.sys_info, MMU::pti(SYS, SYS_INFO), MMU::pti(SYS, SYS_INFO) + 1, Flags::SYS);

    // Set an entry to this page table, so the system can access it later
    sys_pt->remap(si->pmm.sys_pt, MMU::pti(SYS, SYS_PT), MMU::pti(SYS, SYS_PT) + 1, Flags::SYS);

    // System Page Directory
    sys_pt->remap(si->pmm.sys_pd, MMU::pti(SYS, SYS_PD), MMU::pti(SYS, SYS_PD) + 1, Flags::SYS);

    // SYSTEM code
    sys_pt->remap(si->pmm.sys_code, MMU::pti(SYS, SYS_CODE), MMU::pti(SYS, SYS_CODE) + MMU::pages(si->lm.sys_code_size), Flags::SYS);

    // SYSTEM data
    sys_pt->remap(si->pmm.sys_data, MMU::pti(SYS, SYS_DATA), MMU::pti(SYS, SYS_DATA) + MMU::pages(si->lm.sys_data_size), Flags::SYS);

    // SYSTEM stack (used only during init and for the ukernel model)
    sys_pt->remap(si->pmm.sys_stack, MMU::pti(SYS, SYS_STACK), MMU::pti(SYS, SYS_STACK) + MMU::pages(si->lm.sys_stack_size), Flags::SYS);

    // SYSTEM heap is handled by Init_System, so we don't map it here!

    for(unsigned int i = 0; i < MMU::pts(MMU::pages(SYS_HEAP - SYS)); i++)
        db<Setup>(INF) << "SYS_PT[" << &sys_pt[i] << "]=" << sys_pt[i] << endl;
}


void Setup::setup_app_pt()
{
    db<Setup>(TRC) << "Setup::setup_app_pt(appc={b=" << reinterpret_cast<void *>(si->pmm.app_code) << ",s=" << MMU::pages(si->lm.app_code_size) << "}"
                   << ",appd={b=" << reinterpret_cast<void *>(si->pmm.app_data) << ",s=" << MMU::pages(si->lm.app_data_size) << "}"
                   << ",appe={b=" << reinterpret_cast<void *>(si->pmm.app_extra) << ",s=" << MMU::pages(si->lm.app_extra_size) << "}"
                   << "})" << endl;

    // Get the physical address for the first APPLICATION Page Tables, which ware allocated with calloc()
    Page_Table * app_code_pt = reinterpret_cast<Page_Table *>(si->pmm.app_code_pt);
    Page_Table * app_data_pt = reinterpret_cast<Page_Table *>(si->pmm.app_data_pt);

    // APPLICATION code
    // Since load_parts() will load the code into memory, the code segment can't be marked R/O yet
    // The correct flags (APPC and APPD) will be configured after the execution of load_parts(), by adjust_perms()
    app_code_pt->remap(si->pmm.app_code, MMU::pti(si->lm.app_code), MMU::pti(si->lm.app_code) + MMU::pages(si->lm.app_code_size), Flags::APP);

    // APPLICATION data (contains stack, heap and extra)
    app_data_pt->remap(si->pmm.app_data, MMU::pti(si->lm.app_data), MMU::pti(si->lm.app_data) + MMU::pages(si->lm.app_data_size), Flags::APP);

    for(unsigned int i = 0; i < MMU::pts(MMU::pages(si->lm.app_code_size)); i++)
        db<Setup>(INF) << "APPC_PT[" << &app_code_pt[i] << "]=" << app_code_pt[i] << endl;
    for(unsigned int i = 0; i < MMU::pts(MMU::pages(si->lm.app_data_size)); i++)
        db<Setup>(INF) << "APPD_PT[" << &app_data_pt[i] << "]=" << app_data_pt[i] << endl;
}


void Setup::setup_sys_pd()
{
    db<Setup>(TRC) << "Setup::setup_sys_pd(bm="
                   << "{memb="  << reinterpret_cast<void *>(si->bm.mem_base)
                   << ",memt="  << reinterpret_cast<void *>(si->bm.mem_top)
                   << ",miob="  << reinterpret_cast<void *>(si->bm.mio_base)
                   << ",miot="  << reinterpret_cast<void *>(si->bm.mio_top)
                   << ",si="    << reinterpret_cast<void *>(si->pmm.sys_info)
                   << ",spt="   << reinterpret_cast<void *>(si->pmm.sys_pt)
                   << ",spd="   << reinterpret_cast<void *>(si->pmm.sys_pd)
                   << ",mem="   << reinterpret_cast<void *>(si->pmm.phy_mem_pt)
                   << ",io="    << reinterpret_cast<void *>(si->pmm.io_pt)
                   << ",umemb=" << reinterpret_cast<void *>(si->pmm.usr_mem_base)
                   << ",umemt=" << reinterpret_cast<void *>(si->pmm.usr_mem_top)
                   << ",sysc="  << reinterpret_cast<void *>(si->pmm.sys_code)
                   << ",sysd="  << reinterpret_cast<void *>(si->pmm.sys_data)
                   << ",syss="  << reinterpret_cast<void *>(si->pmm.sys_stack)
                   << ",apct="  << reinterpret_cast<void *>(si->pmm.app_code_pt)
                   << ",apdt="  << reinterpret_cast<void *>(si->pmm.app_data_pt)
                   << ",fr1b="  << reinterpret_cast<void *>(si->pmm.free1_base)
                   << ",fr1t="  << reinterpret_cast<void *>(si->pmm.free1_top)
                   << ",fr2b="  << reinterpret_cast<void *>(si->pmm.free2_base)
                   << ",fr2t="  << reinterpret_cast<void *>(si->pmm.free2_top)
                   << "})" << endl;

    // Check alignments
    assert(MMU::pdi(SETUP) == MMU::pdi(RAM_BASE));
    assert(MMU::pdi(INT_M2S) == MMU::pdi(RAM_TOP));
    if(RAM_BASE != MMU::align_segment(RAM_BASE))
        db<Setup>(WRN) << "Setup::setup_sys_pd: unaligned physical memory!" << endl;
    if(PHY_MEM != MMU::align_segment(PHY_MEM))
        db<Setup>(WRN) << "Setup::setup_sys_pd: unaligned physical memory mapping!" << endl;
    if(IO != MMU::align_segment(IO))
        db<Setup>(WRN) << "Setup::setup_sys_pd: unaligned memory-mapped I/O mapping!" << endl;
    if(SYS != MMU::align_segment(SYS))
        db<Setup>(WRN) << "Setup::setup_sys_pd: unaligned OS mapping!" << endl;

    // Get the physical address for the System Page Directory, which was allocated with calloc()
    Directory dir(reinterpret_cast<Page_Directory *>(si->pmm.sys_pd));

    // Map the whole physical memory into the page tables pointed by phy_mem_pt
    Page_Table * mem_pt = reinterpret_cast<Page_Table *>(si->pmm.phy_mem_pt);
    Chunk mem(mem_pt, MMU::pti(si->bm.mem_base), MMU::pti(si->bm.mem_base) + MMU::pages(si->bm.mem_top - si->bm.mem_base), Flags::SYS, si->bm.mem_base);

    // Attach all the physical memory starting at PHY_MEM
    if(dir.attach(mem, PHY_MEM) != PHY_MEM)
        db<Setup>(ERR) << "Setup::setup_sys_pd: cannot attach physical memory at " << reinterpret_cast<void *>(PHY_MEM) << "!" << endl;

    // Also attach the portions of the physical memory used by SETUP (including the BOOT_STACK)
    if(PHY_MEM != RAM_BASE) {
        Chunk base(&mem_pt[MMU::pti(RAM_BASE)], 0, 1, Flags::SYS);
        Chunk top(&mem_pt[MMU::pts(MMU::pti(RAM_BASE, RAM_TOP + 1) - MMU::PT_ENTRIES)], MMU::PT_ENTRIES - 1, MMU::PT_ENTRIES, Flags::SYS);
        if(dir.attach(base, RAM_BASE) != RAM_BASE)
            db<Setup>(ERR) << "Setup::setup_sys_pd: cannot attach SETUP code at " << reinterpret_cast<void *>(RAM_BASE) << "!" << endl;
        if(dir.attach(top, RAM_TOP - sizeof(MMU::Big_Page) + 1) != RAM_TOP - sizeof(MMU::Big_Page) + 1)
            db<Setup>(ERR) << "Setup::setup_sys_pd: cannot attach SETUP stack at " << reinterpret_cast<void *>(RAM_TOP - sizeof(MMU::Big_Page) + 1) << "!" << endl;
    }

    // Map I/O address space into the page tables pointed by io_pt
    Chunk io(si->pmm.io_pt, MMU::pti(si->bm.mio_base), MMU::pti(si->bm.mio_base) + MMU::pages(si->bm.mio_top - si->bm.mio_base), Flags::IO, si->bm.mio_base);

    // Attach devices' memory at Memory_Map::IO
    if(dir.attach(io, IO) != IO)
        db<Setup>(ERR) << "Setup::setup_sys_pd: cannot attach memory-mapped I/O at " << reinterpret_cast<void *>(IO) << "!" << endl;

    // Attach the OS (i.e. sys_pt)
    Chunk sys(si->pmm.sys_pt, MMU::pti(SYS), MMU::pti(SYS) + MMU::pages(SYS_HEAP - SYS), Flags::SYS);
    if(dir.attach(sys, SYS) != SYS)
        db<Setup>(ERR) << "Setup::setup_sys_pd: cannot attach the OS at " << reinterpret_cast<void *>(SYS) << "!" << endl;

    // Attach the first APPLICATION CODE (i.e. app_code_pt)
    Chunk app_code(si->pmm.app_code_pt, MMU::pti(si->lm.app_code), MMU::pti(si->lm.app_code) + MMU::pages(si->lm.app_code_size), Flags::APPC);
    if(dir.attach(app_code, si->lm.app_code) != si->lm.app_code)
        db<Setup>(ERR) << "Setup::setup_sys_pd: cannot attach the application code at " << reinterpret_cast<void *>(si->lm.app_code) << "!" << endl;

    // Attach the first APPLICATION DATA (i.e. app_data_pt, containing heap, stack and extra)
    Chunk app_data(si->pmm.app_data_pt, MMU::pti(si->lm.app_data), MMU::pti(si->lm.app_data) + MMU::pages(si->lm.app_data_size), Flags::APPD);
    if(dir.attach(app_data, si->lm.app_data) != si->lm.app_data)
        db<Setup>(ERR) << "Setup::setup_sys_pd: cannot attach the application data at " << reinterpret_cast<void *>(si->lm.app_data) << "!" << endl;

    // Save free chunks to be passed to MMU::init()
    si->pmm.free1_base = si->bm.mem_base;
    si->pmm.free1_top = si->bm.mem_base + MMU::allocable() * sizeof(Page);

    db<Setup>(INF) << "SYS_PD[" << reinterpret_cast<Page_Directory *>(si->pmm.sys_pd) << "]=" << *reinterpret_cast<Page_Directory *>(si->pmm.sys_pd) << endl;
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


void Setup::load_parts()
{
    db<Setup>(TRC) << "Setup::load_parts()" << endl;

    // Adjust bi to its logical address
    bi = static_cast<char *>((RAM_BASE == PHY_MEM) ? bi : (RAM_BASE > PHY_MEM) ? bi - (RAM_BASE - PHY_MEM) : bi + (PHY_MEM - RAM_BASE));

    // Relocate System_Info
    if(sizeof(System_Info) > sizeof(Page))
        db<Setup>(WRN) << "System_Info is bigger than a page (" << sizeof(System_Info) << ")!" << endl;
    if(Traits<Setup>::hysterically_debugged) {
        db<Setup>(INF) << "Setup:BOOT_IMAGE: " << MMU::Translation(bi, true) << endl;
        db<Setup>(INF) << "Setup:SYS_INFO[phy]: " << MMU::Translation(si) << endl;
        db<Setup>(INF) << "Setup:SYS_INFO[log]: " << MMU::Translation(SYS_INFO) << endl;
    }
    memcpy(reinterpret_cast<System_Info *>(SYS_INFO), si, sizeof(System_Info));
    si = reinterpret_cast<System_Info *>(SYS_INFO);

    // Load INIT
    if(si->lm.has_ini) {
        db<Setup>(TRC) << "Setup::load_init()" << endl;
        ELF * ini_elf = reinterpret_cast<ELF *>(&bi[si->bm.init_offset]);

        if(!ini_elf->valid())
            db<Setup>(ERR) << "INIT ELF image is corrupted!" << endl;

        ELF::Loadable * ini_loadable = reinterpret_cast<ELF::Loadable *>(&si->lm.ini_entry);

        if(Traits<Setup>::hysterically_debugged) {
            db<Setup>(INF) << "Setup:ini_elf: " << MMU::Translation(ini_elf) << endl;
            db<Setup>(INF) << "Setup:ini_elf[CODE]: " << MMU::Translation(ini_loadable->code) << endl;
        }

        ini_elf->load(ini_loadable);
    }

    // Load SYSTEM
    if(si->lm.has_sys) {
        db<Setup>(TRC) << "Setup::load_sys()" << endl;
        ELF * sys_elf = reinterpret_cast<ELF *>(&bi[si->bm.system_offset]);
        if(!sys_elf->valid())
            db<Setup>(ERR) << "OS ELF image is corrupted!" << endl;

        ELF::Loadable * sys_loadable = reinterpret_cast<ELF::Loadable *>(&si->lm.sys_entry);
        if(Traits<Setup>::hysterically_debugged) {
            db<Setup>(INF) << "Setup:sys_elf: " << MMU::Translation(sys_elf) << endl;
            db<Setup>(INF) << "Setup:sys_elf[CODE]: " << MMU::Translation(sys_loadable->code) << endl;
            db<Setup>(INF) << "Setup:sys_elf[DATA]: " << MMU::Translation(sys_loadable->data) << endl;
        }

        sys_elf->load(sys_loadable);
    }

    // Load APP
    if(si->lm.has_app) {
        db<Setup>(TRC) << "Setup::load_app()" << endl;
        ELF * app_elf = reinterpret_cast<ELF *>(&bi[si->bm.application_offset]);
        if(!app_elf->valid())
            db<Setup>(ERR) << "APP ELF image is corrupted!" << endl;

        ELF::Loadable * app_loadable = reinterpret_cast<ELF::Loadable *>(&si->lm.app_entry);
        if(Traits<Setup>::hysterically_debugged) {
            db<Setup>(INF) << "Setup:app_elf: " << MMU::Translation(app_elf) << endl;
            db<Setup>(INF) << "Setup:app_elf[CODE]: " << MMU::Translation(app_loadable->code) << endl;
            db<Setup>(INF) << "Setup:app_elf[DATA]: " << MMU::Translation(app_loadable->data) << endl;
        }

        app_elf->load(app_loadable);
    }

    // Load EXTRA
    if(si->lm.has_ext) {
        db<Setup>(TRC) << "Setup::load_extra()" << endl;
        if(Traits<Setup>::hysterically_debugged)
            db<Setup>(INF) << "Setup:app_ext:" << MMU::Translation(si->lm.app_extra) << endl;
        memcpy(Log_Addr(si->lm.app_extra), &bi[si->bm.extras_offset], si->lm.app_extra_size);
    }
}


void Setup::adjust_perms()
{
    db<Setup>(TRC) << "Setup::adjust_perms(appc={b=" << reinterpret_cast<void *>(si->pmm.app_code) << ",s=" << MMU::pages(si->lm.app_code_size) << "}"
                   << ",appd={b=" << reinterpret_cast<void *>(si->pmm.app_data) << ",s=" << MMU::pages(si->lm.app_data_size) << "}"
                   << ",appe={b=" << reinterpret_cast<void *>(si->pmm.app_extra) << ",s=" << MMU::pages(si->lm.app_extra_size) << "}"
                   << "})" << endl;

    // Get the logical address of the first APPLICATION Page Tables
    Page_Table * app_code_pt = reinterpret_cast<Page_Table *>(si->pmm.app_code_pt);
    Page_Table * app_data_pt = reinterpret_cast<Page_Table *>(si->pmm.app_data_pt);

    // APPLICATION data (contains stack, heap and extra)
    app_code_pt->reflag(MMU::pti(si->lm.app_code), MMU::pti(si->lm.app_code) + MMU::pages(si->lm.app_code_size), Flags::APPC);
    app_data_pt->reflag(MMU::pti(si->lm.app_data), MMU::pti(si->lm.app_data) + MMU::pages(si->lm.app_data_size), Flags::APPD);
}


void Setup::call_next()
{
    // Check for next stage and obtain the entry point
    Log_Addr pc;

    pc = &_start;
    db<Setup>(TRC) << "Setup::call_next(pc=" << pc << ",sp=" << CPU::sp() << ") => APPLICATION" << endl;

    db<Setup>(INF) << "SETUP ends here!" << endl;

    static_cast<void (*)()>(pc)();

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

    CPU::mstatusc(CPU::MIE);                            // disable interrupts (they will be reenabled at Init_End)

    CPU::tp(CPU::mhartid() - 1);                        // tp will be CPU::id() for supervisor mode; we won't count core 0, which is an heterogeneous E51
    CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE - sizeof(long)); // set the stack pointer, thus creating a stack for SETUP

    Machine::clear_bss();

    CPU::mtvec(CPU::INT_DIRECT, Memory_Map::INT_M2S);   // setup a machine mode interrupt handler to forward timer interrupts (which cannot be delegated via mideleg)
    CPU::mideleg(CPU::SSI | CPU::STI | CPU::SEI);       // delegate supervisor interrupts to supervisor mode
    CPU::medeleg(0xf1ff);                               // delegate all exceptions to supervisor mode but ecalls
    CPU::mie(CPU::MSI | CPU::MTI | CPU::MEI);           // enable interrupt generation by at machine level before going into supervisor mode
    CLINT::mtimecmp(-1ULL);                             // configure MTIMECMP so it won't trigger a timer interrupt before we can setup_m2s()
    CPU::mstatus(CPU::MPP_S | CPU::MPIE | CPU::MXR);    // prepare jump into supervisor mode at MRET with interrupts enabled at machine level
    CPU::mstatusc(CPU::SIE);                            // disable interrupts (they will be reenabled at Init_End)
    CPU::sstatuss(CPU::SUM);                            // allows User Memory access in supervisor mode

    CPU::pmpcfg0(0b11111); 				// configure PMP region 0 as (L=unlocked [0], [00], A = NAPOT [11], X [1], W [1], R [1])
    CPU::pmpaddr0((1ULL << MMU::LA_BITS) - 1);          // comprising the whole memory space

    CPU::mepc(CPU::Reg(&_setup));                       // entry = _setup
    CPU::mret();                                        // enter supervisor mode at setup (mepc) with interrupts enabled (mstatus.mpie = true)
}

void _setup() // supervisor mode
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
        "       sd       a2, -12(sp)            \n"
        "       sd       a3, -20(sp)            \n"
        "       sd       a4, -28(sp)            \n"
        "       sd       a5, -36(sp)            \n");
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
    ASM("       ld       a2, -12(sp)            \n"
        "       ld       a3, -20(sp)            \n"
        "       ld       a4, -28(sp)            \n"
        "       ld       a5, -36(sp)            \n");
}
    ASM("       csrr     sp, mscratch           \n"
        "       mret                            \n");
}

