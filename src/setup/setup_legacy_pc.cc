// EPOS PC SETUP

// SETUP is responsible for bringing the machine into a usable state. It configures
// several IA32 data structures (IDT, GDT, etc), builds a basic flat memory model and
// a basic thread model (single-task/single-thread).

// PC_BOOT assumes offset "0" to be the entry point of PC_SETUP. We achieve this by declaring _start in segment .init

#include <architecture.h>
#include <machine.h>
#include <utility/elf.h>
#include <utility/string.h>

extern "C" {
    void _start();
    void _init();

    // SETUP entry point is in .init (and not in .text), so it will be linked first and will be the first function after the ELF header in the image
    void _entry() __attribute__ ((section(".init")));
    void _setup(char * bi);

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
    static const unsigned long SETUP            = Memory_Map::SETUP;
    static const unsigned long APIC_PHY         = APIC::LOCAL_APIC_PHY_ADDR;
    static const unsigned long APIC_SIZE        = APIC::LOCAL_APIC_SIZE;
    static const unsigned long IO_APIC_PHY      = APIC::IO_APIC_PHY_ADDR;
    static const unsigned long IO_APIC_SIZE     = APIC::IO_APIC_SIZE;
    static const unsigned long VGA_PHY          = VGA::FB_PHY_ADDR;
    static const unsigned long VGA_SIZE         = VGA::FB_SIZE;

    // Logical memory map
    static const unsigned long IDT              = Memory_Map::IDT;
    static const unsigned long GDT              = Memory_Map::GDT;
    static const unsigned long TSS0             = Memory_Map::TSS0;
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
    static const unsigned long SYS_HIGH         = Memory_Map::SYS_HIGH;
    static const unsigned long APP_CODE         = Memory_Map::APP_CODE;
    static const unsigned long APP_DATA         = Memory_Map::APP_DATA;

    // Architecture Imports
    typedef CPU::Reg Reg;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;
    typedef CPU::GDT_Entry GDT_Entry;
    typedef CPU::IDT_Entry IDT_Entry;
    typedef CPU::TSS TSS;
    typedef MMU::Page_Flags Flags;
    typedef MMU::Page Page;
    typedef MMU::Page_Table Page_Table;
    typedef MMU::Page_Directory Page_Directory;
    typedef MMU::PT_Entry PT_Entry;
    typedef MMU::PD_Entry PD_Entry;

public:
    Setup(char * boot_image);

private:
    void build_lm();
    void build_pmm();

    void say_hi();

    void setup_idt();
    void setup_gdt();
    void setup_sys_pt();
    void setup_app_pt();
    void setup_sys_pd();
    
    void enable_paging();

    void load_parts();
    void adjust_perms();
    void call_next();

    void detect_memory(unsigned long * base, unsigned long * top);
    void detect_pci(unsigned long * base, unsigned long * top);
    void calibrate_timers();

    static void panic() { Machine::panic(); }

private:
    char * bi;
    System_Info * si;

    static volatile bool paging_ready;
};

volatile bool Setup::paging_ready = false;

Setup::Setup(char * boot_image)
{
    CPU::int_disable(); // interrupts will be re-enabled at init_end

    // SETUP doesn't handle global constructors, so we need to manually initialize any object with a non-empty default constructor
    new (&kout) OStream;
    new (&kerr) OStream;

    Display::init();
    VGA::init(VGA_PHY); // Display can be Serial_Display, so VGA here!
    kout << endl;

    // Get the previously loaded and relocated boot imaged
    bi = reinterpret_cast<char *>(boot_image);
    si = reinterpret_cast<System_Info *>(&__boot_time_system_info);
    if(si->bm.n_cpus > Traits<Machine>::CPUS)
        si->bm.n_cpus = Traits<Machine>::CPUS;

    db<Setup>(TRC) << "Setup(bi=" << reinterpret_cast<void *>(bi) << ",sp=" << CPU::sp() << ")" << endl;
    db<Setup>(INF) << "Setup:si=" << *si << endl;

    // Disable hardware interrupt triggering at PIC
    i8259A::reset();

    // Detect RAM
    unsigned long memb, memt;
    detect_memory(&memb, &memt);

    // Detect PCI devices and calculate PCI apperture
    detect_pci(&si->bm.mio_base, &si->bm.mio_top);

    // Calibrate timers
    calibrate_timers();

    // Build the memory model
    build_lm();
    build_pmm();

    // Print basic facts about this EPOS instance
    say_hi();

    // Configure the memory model defined above
    setup_idt();
    setup_gdt();
    setup_sys_pt();
    setup_app_pt();
    setup_sys_pd();

    // Enable paging
    // We won't be able to print anything before the remap() bellow
    db<Setup>(INF) << "Setup::pc=" << CPU::pc() << endl;
    db<Setup>(INF) << "Setup::sp=" << CPU::sp() << endl;
    db<Setup>(INF) << "Setup::cr0=" << reinterpret_cast<void *>(CPU::cr0()) << endl;
    db<Setup>(INF) << "Setup::cr3=" << reinterpret_cast<void *>(CPU::cr3()) << endl;
    enable_paging();

    // Load EPOS parts (e.g. INIT, SYSTEM, APP) and adjust pointers that will still be used to their logical addresses
    load_parts();

    db<Setup>(INF) << "Setup::pc=" << CPU::pc() << endl;
    db<Setup>(INF) << "Setup::sp=" << CPU::sp() << endl;
    db<Setup>(INF) << "Setup::cr0=" << reinterpret_cast<void *>(CPU::cr0()) << endl;
    db<Setup>(INF) << "Setup::cr3=" << reinterpret_cast<void *>(CPU::cr3()) << endl;

    // SETUP ends here, so let's transfer control to the next stage (INIT or APP)
    call_next();
}


void Setup::build_lm()
{
    db<Setup>(TRC) << "Setup::build_lm()" << endl;

    // Get boot image structure
    si->lm.has_stp = (si->bm.setup_offset != -1u);
    si->lm.has_ini = (si->bm.init_offset != -1u);
    si->lm.has_sys = (si->bm.system_offset != -1u);
    si->lm.has_app = (si->bm.application_offset != -1u);
    si->lm.has_ext = (si->bm.extras_offset != -1u);

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
    si->lm.sys_stack_size = Traits<System>::STACK_SIZE * si->bm.n_cpus;
    if(si->lm.has_sys) {
        ELF * sys_elf = reinterpret_cast<ELF *>(&bi[si->bm.system_offset]);
        if(!sys_elf->valid())
            db<Setup>(ERR) << "OS ELF image is corrupted!" << endl;

        sys_elf->scan(reinterpret_cast<ELF::Loadable *>(&si->lm.sys_entry), SYS_CODE, SYS_DATA - 1, SYS_DATA, SYS_HIGH);

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
        if(si->lm.app_data == ~0UL) {
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
    // Allocate (reserve) memory for all entities we have to setup.
    // We'll start at the highest address to make possible a memory model
    // on which the application's logical and physical address spaces match.

    Phy_Addr top_page = MMU::pages(si->bm.mem_top);

    db<Setup>(TRC) << "Setup::build_pmm() [top=" << reinterpret_cast<void *>(top_page * sizeof(Page)) << "]" << endl;

    // IDT (1 x sizeof(Page))
    top_page -= 1;
    si->pmm.idt = top_page * sizeof(Page);

    // GDT (1 x sizeof(Page))
    top_page -= 1;
    si->pmm.gdt = top_page * sizeof(Page);

    // TSSs (1 x sizeof(Page) x CPUs)
    top_page -= Traits<Machine>::CPUS;
    si->pmm.tss = top_page * sizeof(Page);

    // System Page Directory (1 x sizeof(Page))
    top_page -= 1;
    si->pmm.sys_pd = top_page * sizeof(Page);

    // System Page Table (1 x sizeof(Page))
    top_page -= 1;
    si->pmm.sys_pt = top_page * sizeof(Page);

    // Page tables to map the whole physical memory
    // = NP/NPTE_PT * sizeof(Page)
    //   NP = size of physical memory in pages
    //   NPTE_PT = number of page table entries per page table
    top_page -= MMU::pts(MMU::pages(si->bm.mem_top - si->bm.mem_base));
    si->pmm.phy_mem_pt = top_page * sizeof(Page);

    // Page tables to map the IO address space
    // = NP/NPTE_PT * sizeof(Page)
    //   NP = size of PCI address space in pages
    //   NPTE_PT = number of page table entries per page table
    unsigned int io_size = MMU::pages(si->bm.mio_top - si->bm.mio_base);
    io_size += APIC_SIZE / sizeof(Page); // Add room for APIC (4 kB, 1 page)
    io_size += IO_APIC_SIZE / sizeof(Page); // Add room for IO_APIC (4 kB, 1 page)
    io_size += VGA_SIZE / sizeof(Page); // Add room for VGA (32 kB, 8 pages)
    top_page -= MMU::pts(io_size);
    si->pmm.io_pt = top_page * sizeof(Page);

    // Page tables to map the first APPLICATION code segment
    top_page -= MMU::pts(MMU::pages(si->lm.app_code_size));
    si->pmm.app_code_pt = top_page * sizeof(Page);

    // Page tables to map the first APPLICATION data segment (which contains heap, stack and extra)
    top_page -= MMU::pts(MMU::pages(si->lm.app_data_size));
    si->pmm.app_data_pt = top_page * sizeof(Page);

    // System Info (1 x sizeof(Page))
    top_page -= 1;
    si->pmm.sys_info = top_page * sizeof(Page);

    // SYSTEM code segment
    top_page -= MMU::pages(si->lm.sys_code_size);
    si->pmm.sys_code = top_page * sizeof(Page);

    // SYSTEM data segment
    top_page -= MMU::pages(si->lm.sys_data_size);
    si->pmm.sys_data = top_page * sizeof(Page);

    // SYSTEM stack segment
    top_page -= MMU::pages(si->lm.sys_stack_size);
    si->pmm.sys_stack = top_page * sizeof(Page);

    // The memory allocated so far will "disappear" from the system as we set usr_mem_top as follows:
    si->pmm.usr_mem_base = si->bm.mem_base;
    si->pmm.usr_mem_top = top_page * sizeof(Page);

    // APPLICATION code segment
    top_page -= MMU::pages(si->lm.app_code_size);
    si->pmm.app_code = top_page * sizeof(Page);

    // APPLICATION data segment (contains stack, heap and extra)
    top_page -= MMU::pages(si->lm.app_data_size);
    si->pmm.app_data = top_page * sizeof(Page);

    // Free chunks (passed to MMU::init)
    si->pmm.free1_base = MMU::align_page(si->bm.mem_base);
    si->pmm.free1_top = MMU::align_page(640 * 1024);
    // Skip VRAM and ROMs
    si->pmm.free2_base = MMU::align_page(1024 * 1024);
    si->pmm.free2_top = top_page * sizeof(Page);
    si->pmm.free3_base =0;
    si->pmm.free3_top = 0;

    // Test if we didn't overlap SETUP and the boot image
    if(si->pmm.usr_mem_top <= si->lm.stp_code + si->lm.stp_code_size + si->lm.stp_data_size)
        db<Setup>(ERR) << "SETUP would have been overwritten!" << endl;
}


void Setup::say_hi()
{
    db<Setup>(TRC) << "Setup::say_hi()" << endl;
    db<Setup>(INF) << "System_Info=" << *si << endl;

    if(!si->lm.has_app)
        db<Setup>(ERR) << "No APPLICATION in boot image, you don't need EPOS!" << endl;
    if(!si->lm.has_sys)
        db<Setup>(INF) << "No SYSTEM in boot image, assuming EPOS is a library!" << endl;

    kout << "\nThis is EPOS!\n" << endl;
    kout << "Setting up this machine as follows: " << endl;
    kout << "  Mode:         " << ((Traits<Build>::MODE == Traits<Build>::LIBRARY) ? "library" : (Traits<Build>::MODE == Traits<Build>::BUILTIN) ? "built-in" : "kernel") << endl;
    kout << "  Processor:    " << Traits<Machine>::CPUS << " x IA32 at " << Traits<CPU>::CLOCK / 1000000 << " MHz (BUS clock = " << Traits<CPU>::CLOCK / 1000000 << " MHz)" << endl;
    kout << "  Machine:      PC" << endl;
    kout << "  Memory:       " << (si->bm.mem_top - si->bm.mem_base) / 1024 << " KB [" << reinterpret_cast<void *>(si->bm.mem_base) << ":" << reinterpret_cast<void *>(si->bm.mem_top) << "]" << endl;
    kout << "  User memory:  " << (si->pmm.usr_mem_top - si->pmm.usr_mem_base) / 1024 << " KB [" << reinterpret_cast<void *>(si->pmm.usr_mem_base) << ":" << reinterpret_cast<void *>(si->pmm.usr_mem_top) << "]" << endl;
    kout << "  I/O space:    " << (si->bm.mio_top - si->bm.mio_base) / 1024 << " KB [" << reinterpret_cast<void *>(si->bm.mio_base) << ":" << reinterpret_cast<void *>(si->bm.mio_top) << "]" << endl;
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


void Setup::setup_idt()
{
    db<Setup>(TRC) << "setup_idt(idt=" << (void *)si->pmm.idt << ")" << endl;

    // Get the physical address for the IDT
    IDT_Entry * idt = reinterpret_cast<IDT_Entry *>((void *)si->pmm.idt);

    // Clear IDT
    memset(idt, 0, sizeof(Page));

    // Adjust handler addresses to logical addresses
    Log_Addr panic_h = Log_Addr(&panic) | PHY_MEM;

    // Map all handlers to panic()
    for(unsigned int i = 0; i < CPU::IDT_ENTRIES; i++)
        idt[i] = IDT_Entry(CPU::GDT_SYS_CODE, panic_h, CPU::SEG_IDT_ENTRY);

    db<Setup>(INF) << "IDT[0  ]=" << idt[0] << " (" << panic_h << ")" << endl;
    db<Setup>(INF) << "IDT[255]=" << idt[255] << " (" << panic_h << ")" << endl;
}


void Setup::setup_gdt()
{
    db<Setup>(TRC) << "setup_gdt(gdt=" << (void *)si->pmm.gdt << ")" << endl;

    // Get the physical address for the GDT
    GDT_Entry * gdt = reinterpret_cast<GDT_Entry *>((void *)si->pmm.gdt);

    // Clear GDT
    memset(gdt, 0, sizeof(Page));

    // GDT_Entry(base, limit, {P,DPL,S,TYPE})
    gdt[CPU::GDT_NULL]      = GDT_Entry(0,        0, 0);
    gdt[CPU::GDT_FLT_CODE]  = GDT_Entry(0,  0xfffff, CPU::SEG_FLT_CODE);
    gdt[CPU::GDT_FLT_DATA]  = GDT_Entry(0,  0xfffff, CPU::SEG_FLT_DATA);
    gdt[CPU::GDT_APP_CODE]  = GDT_Entry(0,  0xfffff, CPU::SEG_APP_CODE);
    gdt[CPU::GDT_APP_DATA]  = GDT_Entry(0,  0xfffff, CPU::SEG_APP_DATA);
    for(unsigned int i = 0; i < Traits<Machine>::CPUS; i++)
        gdt[CPU::GDT_TSS0 + i] = GDT_Entry(TSS0 + i * sizeof(Page), 0xfff, CPU::SEG_TSS0);

    db<Setup>(INF) << "GDT[NULL=" << CPU::GDT_NULL     << "]=" << gdt[CPU::GDT_NULL] << endl;
    db<Setup>(INF) << "GDT[SYCD=" << CPU::GDT_SYS_CODE << "]=" << gdt[CPU::GDT_SYS_CODE] << endl;
    db<Setup>(INF) << "GDT[SYDT=" << CPU::GDT_SYS_DATA << "]=" << gdt[CPU::GDT_SYS_DATA] << endl;
    db<Setup>(INF) << "GDT[APCD=" << CPU::GDT_APP_CODE << "]=" << gdt[CPU::GDT_APP_CODE] << endl;
    db<Setup>(INF) << "GDT[APDT=" << CPU::GDT_APP_DATA << "]=" << gdt[CPU::GDT_APP_DATA] << endl;
    for(unsigned int i = 0; i < Traits<Machine>::CPUS; i++)
        db<Setup>(INF) << "GDT[TSS" << i << "=" << CPU::GDT_TSS0  + i << "]=" << gdt[CPU::GDT_TSS0 + i] << endl;
}


void Setup::setup_sys_pt()
{
    db<Setup>(TRC) << "Setup::setup_sys_pt(pmm="
                   << "{idt="     << reinterpret_cast<void *>(si->pmm.idt)
                   << ",gdt="     << reinterpret_cast<void *>(si->pmm.gdt)
                   << ",si="      << reinterpret_cast<void *>(si->pmm.sys_info)
                   << ",pt="      << reinterpret_cast<void *>(si->pmm.sys_pt)
                   << ",pd="      << reinterpret_cast<void *>(si->pmm.sys_pd)
                   << ",sysc={b=" << reinterpret_cast<void *>(si->pmm.sys_code) << ",s=" << MMU::pages(si->lm.sys_code_size) << "}"
                   << ",sysd={b=" << reinterpret_cast<void *>(si->pmm.sys_data) << ",s=" << MMU::pages(si->lm.sys_data_size) << "}"
                   << ",syss={b=" << reinterpret_cast<void *>(si->pmm.sys_stack) << ",s=" << MMU::pages(si->lm.sys_stack_size) << "}"
                   << "})" << endl;

    // Get the physical address for the SYSTEM Page Table
    PT_Entry * sys_pt = reinterpret_cast<PT_Entry *>(si->pmm.sys_pt);
    unsigned int pts = MMU::pts(MMU::pages(SYS_HIGH - SYS));

    // Clear the System Page Table
    memset(sys_pt, 0, pts * sizeof(Page_Table));

    // IDT
    sys_pt[MMU::pti(SYS, IDT)] = si->pmm.idt | Flags::SYS;

    // GDT
    sys_pt[MMU::pti(SYS, GDT)] = si->pmm.gdt | Flags::SYS;

    // TSSs
    for(unsigned int i = 0; i < Traits<Machine>::CPUS; i++)
        sys_pt[MMU::pti(SYS, TSS0) + i] = (si->pmm.tss + i * sizeof(Page)) | Flags::SYS;

    // System Info
    sys_pt[MMU::pti(SYS, SYS_INFO)] = MMU::phy2pte(si->pmm.sys_info, Flags::SYS);

    // Set an entry to this page table, so the system can access it later
    sys_pt[MMU::pti(SYS, SYS_PT)] = MMU::phy2pte(si->pmm.sys_pt, Flags::SYS);

    // System Page Directory
    sys_pt[MMU::pti(SYS, SYS_PD)] = MMU::phy2pte(si->pmm.sys_pd, Flags::SYS);

    unsigned int i;
    PT_Entry aux;

    // SYSTEM code
    for(i = 0, aux = si->pmm.sys_code; i < MMU::pages(si->lm.sys_code_size); i++, aux = aux + sizeof(Page))
        sys_pt[MMU::pti(SYS, SYS_CODE) + i] = MMU::phy2pte(aux, Flags::SYS);

    // SYSTEM data
    for(i = 0, aux = si->pmm.sys_data; i < MMU::pages(si->lm.sys_data_size); i++, aux = aux + sizeof(Page))
        sys_pt[MMU::pti(SYS, SYS_DATA) + i] = MMU::phy2pte(aux, Flags::SYS);

    // SYSTEM stack (used only during init and for the ukernel model)
    for(i = 0, aux = si->pmm.sys_stack; i < MMU::pages(si->lm.sys_stack_size); i++, aux = aux + sizeof(Page))
        sys_pt[MMU::pti(SYS, SYS_STACK) + i] = MMU::phy2pte(aux, Flags::SYS);

    // SYSTEM heap is handled by Init_System, so we don't map it here!

    for(unsigned int i = 0; i < pts; i++)
        db<Setup>(INF) << "SYS_PT[" << &sys_pt[i * MMU::PT_ENTRIES] << "]=" << *reinterpret_cast<Page_Table *>(&sys_pt[i * MMU::PT_ENTRIES]) << endl;
}


void Setup::setup_app_pt()
{
    db<Setup>(TRC) << "Setup::setup_app_pt(appc={b=" << reinterpret_cast<void *>(si->pmm.app_code) << ",s=" << MMU::pages(si->lm.app_code_size) << "}"
                   << ",appd={b=" << reinterpret_cast<void *>(si->pmm.app_data) << ",s=" << MMU::pages(si->lm.app_data_size) << "}"
                   << ",appe={b=" << reinterpret_cast<void *>(si->pmm.app_extra) << ",s=" << MMU::pages(si->lm.app_extra_size) << "}"
                   << "})" << endl;

    // Get the physical address for the first APPLICATION Page Tables
    PT_Entry * app_code_pt = reinterpret_cast<PT_Entry *>(si->pmm.app_code_pt);
    PT_Entry * app_data_pt = reinterpret_cast<PT_Entry *>(si->pmm.app_data_pt);
    unsigned int n_pts_code = MMU::pts(MMU::pages(si->lm.app_code_size));
    unsigned int n_pts_data = MMU::pts(MMU::pages(si->lm.app_data_size));

    // Clear the first APPLICATION Page Tables
    memset(app_code_pt, 0, n_pts_code * sizeof(Page_Table));
    memset(app_data_pt, 0, n_pts_data * sizeof(Page_Table));

    unsigned int i;
    PT_Entry aux;

    // APPLICATION code
    // Since load_parts() will load the code into memory, the code segment can't be marked R/O yet
    // The correct flags (APPC and APPD) will be configured after the execution of load_parts(), by adjust_perms()
    for(i = 0, aux = si->pmm.app_code; i < MMU::pages(si->lm.app_code_size); i++, aux = aux + sizeof(Page))
        app_code_pt[MMU::pti(si->lm.app_code) + i] = MMU::phy2pte(aux, Flags::APP);

    // APPLICATION data (contains stack, heap and extra)
    for(i = 0, aux = si->pmm.app_data; i < MMU::pages(si->lm.app_data_size); i++, aux = aux + sizeof(Page))
        app_data_pt[MMU::pti(si->lm.app_data) + i] = MMU::phy2pte(aux, Flags::APP);

    for(unsigned int i = 0; i < n_pts_code; i++)
        db<Setup>(INF) << "APPC_PT[" << &app_code_pt[i * MMU::PT_ENTRIES] << "]=" << *reinterpret_cast<Page_Table *>(&app_code_pt[i * MMU::PT_ENTRIES]) << endl;
    for(unsigned int i = 0; i < n_pts_data; i++)
        db<Setup>(INF) << "APPD_PT[" << &app_data_pt[i * MMU::PT_ENTRIES] << "]=" << *reinterpret_cast<Page_Table *>(&app_data_pt[i * MMU::PT_ENTRIES]) << endl;
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

    // Get the physical address for the System Page Directory
    PT_Entry * sys_pd = reinterpret_cast<PT_Entry *>(si->pmm.sys_pd);

    // Clear the System Page Directory
    memset(sys_pd, 0, sizeof(Page_Directory));

    // Calculate the number of page tables needed to map the physical memory
    unsigned int mem_size = MMU::pages(si->bm.mem_top - si->bm.mem_base);
    unsigned int pts = MMU::pts(mem_size);

    // Map the whole physical memory into the page tables pointed by phy_mem_pt
    PT_Entry * pt = reinterpret_cast<PT_Entry *>(si->pmm.phy_mem_pt);
    for(unsigned int i = MMU::pti(si->bm.mem_base), j = 0; i < MMU::pti(si->bm.mem_base) + mem_size; i++, j++)
        pt[i] = MMU::phy2pte(si->bm.mem_base + j * sizeof(Page), Flags::APP);

    // Attach all the physical memory starting at PHY_MEM
    assert((MMU::pdi(MMU::align_segment(PHY_MEM)) + pts) < (MMU::PD_ENTRIES - 1)); // check if it would overwrite the OS
    for(unsigned int i = MMU::pdi(MMU::align_segment(PHY_MEM)), j = 0; i < MMU::pdi(MMU::align_segment(PHY_MEM)) + pts; i++, j++)
        sys_pd[i] = MMU::phy2pde(si->pmm.phy_mem_pt + j * sizeof(Page_Table), Flags::SYS);

    // Attach all the physical memory starting at RAM_BASE (used in library mode)
    assert((MMU::pdi(MMU::align_segment(RAM_BASE)) + pts) < (MMU::PD_ENTRIES - 1)); // check if it would overwrite the OS
    if(RAM_BASE != PHY_MEM)
        for(unsigned int i = MMU::pdi(MMU::align_segment(RAM_BASE)), j = 0; i < MMU::pdi(MMU::align_segment(RAM_BASE)) + pts; i++, j++)
            sys_pd[i] = MMU::phy2pde(si->pmm.phy_mem_pt + j * sizeof(Page_Table), Flags::APP);

    // Calculate the number of page tables needed to map the IO address space
    unsigned int io_size = MMU::pages(si->bm.mio_top - si->bm.mio_base);
    io_size += APIC_SIZE / sizeof(Page); // add room for APIC (4 kB, 1 page)
    io_size += VGA_SIZE / sizeof(Page); // add room for VGA (64 kB, 16 pages)
    pts = MMU::pts(io_size);

    // Map I/O address space into the page tables pointed by io_pt
    pt = reinterpret_cast<PT_Entry *>(si->pmm.io_pt);
    unsigned int i = 0;
    for(; i < (APIC_SIZE / sizeof(Page)); i++)
        pt[i] = MMU::phy2pte(APIC_PHY + i * sizeof(Page), Flags::APIC);
    for(unsigned int j = 0; i < ((APIC_SIZE / sizeof(Page)) + (IO_APIC_SIZE / sizeof(Page))); i++, j++)
        pt[i] = MMU::phy2pte(IO_APIC_PHY + j * sizeof(Page), Flags::APIC);
    for(unsigned int j = 0; i < ((APIC_SIZE / sizeof(Page)) + (IO_APIC_SIZE / sizeof(Page)) + (VGA_SIZE / sizeof(Page))); i++, j++)
        pt[i] = MMU::phy2pte(VGA_PHY + j * sizeof(Page), Flags::VGA);
    for(unsigned int j = 0; i < io_size; i++, j++)
        pt[i] = MMU::phy2pte(si->bm.mio_base + j * sizeof(Page), Flags::PCI);

    // Attach devices' memory at Memory_Map::IO
    assert((MMU::pdi(MMU::align_segment(IO)) + pts) < (MMU::PD_ENTRIES - 1)); // check if it would overwrite the OS
    for(unsigned int i = MMU::pdi(MMU::align_segment(IO)), j = 0; i < MMU::pdi(MMU::align_segment(IO)) + pts; i++, j++)
        sys_pd[i] = MMU::phy2pde(si->pmm.io_pt + j * sizeof(Page_Table), Flags::PCI);

    // Attach the OS (i.e. sys_pt)
    pts = MMU::pts(MMU::pages(SYS_HEAP - SYS)); // SYS_HEAP is handled by Init_System
    for(unsigned int i = MMU::pdi(MMU::align_segment(SYS)), j = 0; i < MMU::pdi(MMU::align_segment(SYS)) + pts; i++, j++)
        sys_pd[i] = MMU::phy2pde(si->pmm.sys_pt + j * sizeof(Page_Table), Flags::SYS);

    // Attach the first APPLICATION CODE (i.e. app_code_pt)
    pts = MMU::pts(MMU::pages(si->lm.app_code_size));
    for(unsigned int i = MMU::pdi(MMU::align_segment(si->lm.app_code)), j = 0; i < MMU::pdi(MMU::align_segment(si->lm.app_code)) + pts; i++, j++)
        sys_pd[i] = MMU::phy2pde(si->pmm.app_code_pt + j * sizeof(Page_Table), Flags::APPC);

    // Attach the first APPLICATION DATA (i.e. app_data_pt, containing heap, stack and extra)
    pts = MMU::pts(MMU::pages(si->lm.app_data_size));
    for(unsigned int i = MMU::pdi(MMU::align_segment(si->lm.app_data)), j = 0; i < MMU::pdi(MMU::align_segment(si->lm.app_data)) + pts; i++, j++)
        sys_pd[i] = MMU::phy2pde(si->pmm.app_data_pt + j * sizeof(Page_Table), Flags::APPD);

    db<Setup>(INF) << "SYS_PD[" << sys_pd << "]=" << *reinterpret_cast<Page_Directory *>(sys_pd) << endl;
}


void Setup::enable_paging()
{
    db<Setup>(TRC) << "Setup::enable_paging()" << endl;
    if(Traits<Setup>::hysterically_debugged) {
        db<Setup>(INF) << "Setup::pc=" << CPU::pc() << endl;
        db<Setup>(INF) << "Setup::sp=" << CPU::sp() << endl;
    }

    // Set IDTR (limit = 1 x sizeof(Page))
    CPU::idtr(sizeof(Page) - 1, IDT);

    // Reload GDTR with its linear address (one more absurd from Intel!)
    CPU::gdtr(sizeof(Page) - 1, GDT);

    // Set CR3 (PDBR) register
    MMU::pd(si->pmm.sys_pd);

    // Enable paging
    Reg aux = CPU::cr0();
    aux &= CPU::CR0_CLEAR;
    aux |= CPU::CR0_SET;
    CPU::cr0(aux);

    // The following relative far jump breaks the IA32 prefetch queue, causing the processor to effectively check the changes to IDRT, GDTR, CR3 and CR0
    ASM("ljmp %0, %1 + 1f" : : "i"(CPU::SEL_FLT_CODE), "i"(PHY_MEM));
    ASM("1:");

    // Reload segment registers with GDT_FLT_DATA
    ASM("" : : "a" (CPU::SEL_FLT_DATA));
    ASM("movw %ax, %ds");
    ASM("movw %ax, %es");
    ASM("movw %ax, %fs");
    ASM("movw %ax, %gs");
    ASM("movw %ax, %ss");

    // Set stack pointer to its logical address
    ASM("orl %0, %%esp" : : "i" (PHY_MEM));

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

    // Adjust pointers that will still be used to their logical addresses
    VGA::init(Memory_Map::VGA); // Display can be Serial_Display, so VGA here!
    APIC::remap(Memory_Map::APIC);

    // Relocate System_Info
    if(sizeof(System_Info) > sizeof(Page))
        db<Setup>(WRN) << "System_Info is bigger than a page (" << sizeof(System_Info) << ")!" << endl;
    bi = static_cast<char *>(MMU::phy2log(bi));
    si = reinterpret_cast<System_Info *>(SYS_INFO);
    if(Traits<Setup>::hysterically_debugged) {
        db<Setup>(INF) << "Setup:boot_info: " << MMU::Translation(bi) << endl;
        db<Setup>(INF) << "Setup:system_info: " << MMU::Translation(si) << endl;
    }
    memcpy(si, __boot_time_system_info, sizeof(System_Info));

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
            db<Setup>(INF) << "Setup:ini_elf[DATA]: " << MMU::Translation(ini_loadable->data) << endl;
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
    PT_Entry * app_code_pt = MMU::phy2log(reinterpret_cast<PT_Entry *>(si->pmm.app_code_pt));
    PT_Entry * app_data_pt = MMU::phy2log(reinterpret_cast<PT_Entry *>(si->pmm.app_data_pt));

    unsigned int i;
    PT_Entry aux;

    // APPLICATION code
    for(i = 0, aux = si->pmm.app_code; i < MMU::pages(si->lm.app_code_size); i++, aux = aux + sizeof(Page))
        app_code_pt[MMU::pti(APP_CODE) + i] = MMU::phy2pte(aux, Flags::APPC);

    // APPLICATION data (contains stack, heap and extra)
    for(i = 0, aux = si->pmm.app_data; i < MMU::pages(si->lm.app_data_size); i++, aux = aux + sizeof(Page))
        app_data_pt[MMU::pti(APP_DATA) + i] = MMU::phy2pte(aux, Flags::APPD);
}


void Setup::call_next()
{
    // Check for next stage and obtain the entry point
    Log_Addr pc = si->lm.app_entry;

    // Arrange a stack for each CPU to support stage transition
    // The 2 integers on the stacks are room for the return address
    Log_Addr sp = SYS_STACK + Traits<System>::STACK_SIZE - 2 * sizeof(int);

    db<Setup>(TRC) << "Setup::call_next(pc=" << pc << ",sp=" << sp << ") => APPLICATION" << endl;

    db<Setup>(INF) << "SETUP ends here!" << endl;

    // Set SP and call the next stage
    CPU::sp(sp);
    static_cast<void (*)()>(pc)();

    // This will only happen when INIT was called and Thread was disabled
    // Note we don't have the original stack here anymore!
    reinterpret_cast<void (*)()>(si->lm.app_entry)();

    // SETUP is now part of the free memory and this point should never be reached, but, just in case ... :-)
    db<Setup>(ERR) << "OS failed to init!" << endl;
}


void Setup::detect_memory(unsigned long * base, unsigned long * top)
{
    db<Setup>(TRC) << "Setup::detect_memory()" << endl;

    unsigned int i;
    unsigned long * mem = reinterpret_cast<unsigned long *>(RAM_BASE / sizeof(long));
    for(i = Traits<Machine>::INIT; i < RAM_TOP; i += 16 * sizeof(MMU::Page))
        mem[i /  sizeof(int)] = i;

    for(i = Traits<Machine>::INIT; i < RAM_TOP; i += 16 * sizeof(MMU::Page))
        if(mem[i / sizeof(int)] != i) {
            db<Setup>(ERR) << "Less memory was detected (" << i / 1024 << " kb) than specified in the configuration (" << RAM_TOP / 1024 << " kb)!" << endl;
            break;
        }

    *base = RAM_BASE;
    *top = i;

    db<Setup>(INF) << "Memory={base=" << reinterpret_cast<void *>(*base) << ",top=" << reinterpret_cast<void *>(*top) << "}" << endl;
}


void Setup::detect_pci(unsigned long * base, unsigned long * top)
{
    db<Setup>(TRC) << "Setup::detect_pci()" << endl;

    // Scan the PCI bus looking for devices with memory mapped regions
    *base = ~0U;
    *top = 0U;
    for(int bus = 0; bus <= Traits<PCI>::MAX_BUS; bus++) {
        for(int dev_fn = 0; dev_fn <= Traits<PCI>::MAX_DEV_FN; dev_fn++) {
            PCI::Locator loc(bus, dev_fn);
            PCI::Header hdr;
            PCI::header(loc, &hdr);
            if(hdr) {
                db<Setup>(INF) << "PCI" << hdr << endl;
                for(unsigned int i = 0; i < PCI::Region::N; i++) {
                    PCI::Region * reg = &hdr.region[i];
                    if(*reg) {
                        db<Setup>(INF) << "  reg[" << i << "]=" << *reg << endl;
                        if(reg->memory) {
                            if(reg->phy_addr < *base)
                                *base = reg->phy_addr;
                            if((reg->phy_addr + reg->size) > *top)
                                *top = reg->phy_addr + reg->size;
                        }
                    }
                }
            }
        }
    }

    db<Setup>(INF) << "PCI address space={base=" << reinterpret_cast<void *>(*base) << ",top=" << reinterpret_cast<void *>(*top) << "}" << endl;
}


void Setup::calibrate_timers()
{
    db<Setup>(TRC) << "Setup::calibrate_timers()" << endl;

    // Disable speaker so we can use channel 2 of i8253
    i8255::port_b(i8255::port_b() & ~(i8255::SPEAKER | i8255::I8253_GATE2));

    // Program i8253 channel 2 to count 50 ms
    i8253::config(2, i8253::CLOCK/20, false, false);

    // Enable i8253 channel 2 counting
    i8255::port_b(i8255::port_b() | i8255::I8253_GATE2);

    // Read CPU clock counter
    TSC::Time_Stamp t0 = TSC::time_stamp();

    // Wait for i8253 counting to finish
    while(!(i8255::port_b() & i8255::I8253_OUT2));

    // Read CPU clock counter again
    TSC::Time_Stamp t1 = TSC::time_stamp(); // ascending

    // The measurement was for 50ms, scale it to 1s
    si->tm.cpu_clock = (t1 - t0) * 20;
    db<Setup>(INF) << "Setup::calibrate_timers:CPU clock=" << si->tm.cpu_clock / 1000000 << " MHz" << endl;

    // Disable speaker so we can use channel 2 of i8253
    i8255::port_b(i8255::port_b() & ~(i8255::SPEAKER | i8255::I8253_GATE2));

    // Program i8253 channel 2 to count 50 ms
    i8253::config(2, i8253::CLOCK/20, false, false);

    // Program APIC_Timer to count as long as it can
    APIC_Timer::config(0, APIC_Timer::Count(-1), false, false);

    // Enable i8253 channel 2 counting
    i8255::port_b(i8255::port_b() | i8255::I8253_GATE2);

    // Read APIC_Timer counter
    APIC_Timer::Count t3 = APIC_Timer::read(0); // descending

    // Wait for i8253 counting to finish
    while(!(i8255::port_b() & i8255::I8253_OUT2));

    // Read APIC_Timer counter again
    APIC_Timer::Count t2 = APIC_Timer::read(0);

    si->tm.bus_clock = (t3 - t2) * 20 * 16; // APIC_Timer is prescaled by 16
    db<Setup>(INF) << "Setup::calibrate_timers:BUS clock=" << si->tm.bus_clock / 1000000 << " MHz" << endl;
}

__END_SYS

using namespace EPOS::S;

//========================================================================
// _entry
//
// "_start" MUST BE PC_SETUP's first function, since PC_BOOT assumes
// offset "0" to be the entry point. It is a kind of bridge between the
// assembly world of PC_BOOT and the C++ world of PC_SETUP. It's main
// tasks are:
//
// - reload PC_SETUP from its ELF header (to the address it was compiled
//   for);
// - setup a stack for PC_SETUP (one for each CPU)
// - direct non-boot CPUs into the trampoline code inside PC_BOOT
//
// The initial stack pointer is inherited from PC_BOOT (i.e.,
// somewhere below 0x7c00).
//
// We can't "kout" here because the data segment is unreachable
// and "kout" has static data.
//
// THIS FUNCTION MUST BE RELOCATABLE, because it won't run at the
// address it has been compiled for.
//------------------------------------------------------------------------
void _entry()
{
    // Set EFLAGS
    CPU::flags(CPU::flags() & CPU::FLAG_CLEAR);

    // Disable interrupts
    CPU::int_disable();

    // Initialize the APIC (if present)
    APIC::reset(APIC::LOCAL_APIC_PHY_ADDR);

    // Recover System_Info
    System_Info * si = reinterpret_cast<System_Info *>(&__boot_time_system_info);

    // The boot strap code loaded the boot image either at Traits<Machine>::IMAGE or on a RAMDISK
    char * bi = reinterpret_cast<char *>((si->bm.img_size <= 2880 * 512) ? Traits<Machine>::IMAGE : Traits<Machine>::RAMDISK);

    // Check SETUP integrity and get information about its ELF structure
    ELF * elf = reinterpret_cast<ELF *>(&bi[si->bm.setup_offset]);
    if(!elf->valid())
        Machine::panic();
    char * entry = reinterpret_cast<char *>(elf->entry());

    // Test if we can access the address for which SETUP has been compiled
    *entry = 'G';
    if(*entry != 'G')
        Machine::panic();

    // Load SETUP considering the address in the ELF header
    // Be careful: by reloading SETUP, global variables have been reset to the values stored in the ELF data segment
    // Also check if this wouldn't destroy the boot image
    int size = elf->segment_size(0);
    if(elf->segment_address(0) <= reinterpret_cast<unsigned int>(&bi[si->bm.img_size]))
        Machine::panic();
    if(elf->load_segment(0) < 0)
        Machine::panic();

    // Move the boot image to after SETUP, so there will be nothing else below SETUP to be preserved
    // SETUP code + data + 1 stack per CPU
    register char * dst = MMU::align_page(entry + size + sizeof(MMU::Page));
    memcpy(dst, bi, si->bm.img_size);

    // Setup a single page stack for SETUP after its data segment
    // SP = "entry" + "size" + sizeof(Page)
    // Be careful: we'll loose our old stack now, so everything we still need to reach Setup() must be in regs or globals!
    register char * sp = const_cast<char *>(dst);
    ASM("movl %0, %%esp" : : "r" (sp));

    // Pass the boot image to SETUP
    ASM("pushl %0" : : "r" (dst));

    // Call setup()
    // the assembly is necessary because the compiler generates
    // relative calls and we need an absolute one
    ASM("call *%0" : : "r" (&_setup));
}


void _setup(char * bi)
{
    kerr  << endl;
    kout  << endl;

    Setup setup(bi);
}
