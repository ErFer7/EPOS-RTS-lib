// EPOS RISC-V 32 MMU Mediator Declarations

#ifndef __rv32_mmu_h
#define __rv32_mmu_h

#define __mmu_common_only__
#include <architecture/mmu.h>
#undef __mmu_common_only__
#include <system/memory_map.h>

__BEGIN_SYS

class SV32_MMU: public MMU_Common<10, 10, 12>
{
    friend class CPU;
    friend class Setup;

private:
    typedef Grouping_List<Frame> List;
    typedef MMU_Common<10, 10, 12> Common;

    static const bool colorful = Traits<MMU>::colorful;
    static const unsigned long COLORS = Traits<MMU>::COLORS;
    static const unsigned long RAM_BASE = Memory_Map::RAM_BASE;
    static const unsigned long PHY_MEM = Memory_Map::PHY_MEM;
    static const unsigned long APP_LOW = Memory_Map::APP_LOW;
    static const unsigned long APP_HIGH = Memory_Map::APP_HIGH;

public:
    // Page Flags
    class Page_Flags
    {
    public:
        enum : Reg {
            V    = 1 << 0, // Valid
            R    = 1 << 1, // Readable
            W    = 1 << 2, // Writable
            X    = 1 << 3, // Executable
            U    = 1 << 4, // User accessible
            G    = 1 << 5, // Global (mapped in multiple PTs)
            A    = 1 << 6, // Accessed
            D    = 1 << 7, // Dirty
            CT   = 1 << 8, // Contiguous (reserved for use by supervisor RSW)
            MIO  = 1 << 9, // I/O (reserved for use by supervisor RSW)

            IAD  = (Traits<Build>::MODEL == Traits<Build>::SiFive_U) ? A | D : 0, // SiFive-U RV64 MMU can't handle A and D and requires it to be set

            APP  = (V | R | W | X | U | IAD),
            APPC = (V | R |     X | U | IAD),
            APPD = (V | R | W     | U | IAD),
            SYS  = (V | R | W | X | IAD),
            SYSC = (V | R |     X | IAD),
            SYSD = (V | R | W     | IAD),
            IO   = (SYSD | MIO),
            DMA  = (SYSD | CT),
            MASK = (1 << 10) - 1,

            PT  = V,
            PD  = V,

            FLAT_MEM_PD = SYS,
        };

    public:
        Page_Flags() {}
        Page_Flags(Reg f): _flags(f) {}
        Page_Flags(const Flags & f): _flags(V | IAD |
                                            ((f & Flags::RD)  ? R  : 0) |
                                            ((f & Flags::WR)  ? W  : 0) |
                                            ((f & Flags::EX)  ? X  : 0) |
                                            ((f & Flags::USR) ? U  : 0) |
                                            ((f & Flags::CWT) ? 0  : 0) |
                                            ((f & Flags::CD)  ? 0  : 0) |
                                            ((f & Flags::CT)  ? CT : 0) |
                                            ((f & Flags::IO)  ? IO : 0) ) {}

        operator Reg() const { return _flags; }
        operator Flags() const { return ((_flags & V)   ? Flags::PRE : 0 |
                                         (_flags & R)   ? Flags::RD  : 0 |
                                         (_flags & W)   ? Flags::WR  : 0 |
                                         (_flags & X)   ? Flags::EX  : 0 |
                                         (_flags & U)   ? Flags::USR : 0 |
                                         (_flags & G)   ? Flags::SPE : 0 |
                                         (_flags & A)   ? Flags::AC  : 0 |
                                         (_flags & D)   ? Flags::DT  : 0 |
                                         (_flags & CT)  ? Flags::CT  : 0 |
                                         (_flags & MIO) ? Flags::IO  : 0 );
        }

        friend OStream & operator<<(OStream & os, const Page_Flags & f) { os << Flags(f._flags); return os; }

    private:
        Reg _flags;
    };

    // Page Table
    class Page_Table
    {
    public:
        Page_Table() {}

        PT_Entry & operator[](unsigned int i) { return _entry[i]; }
        Page_Table & log() { return *static_cast<Page_Table *>(phy2log(this)); }

        void map(int from, int to, Page_Flags flags, Color color) {
            Phy_Addr * addr = alloc(to - from, color);
            if(addr)
                remap(addr, from, to, flags);
            else
                for( ; from < to; from++) {
                    Log_Addr * pte = phy2log(&_entry[from]);
                    *pte = phy2pte(alloc(1, color), flags);
                }
        }

        void map_contiguous(int from, int to, Page_Flags flags, Color color) {
            remap(alloc(to - from, color), from, to, flags);
        }

        void remap(Phy_Addr addr, int from, int to, Page_Flags flags) {
            addr = align_page(addr);
            for( ; from < to; from++) {
                Log_Addr * pte = phy2log(&_entry[from]);
                *pte = phy2pte(addr, flags);
                addr += sizeof(Page);
            }
        }

        void reflag(int from, int to, Page_Flags flags) {
            for( ; from < to; from++) {
                Log_Addr * pte = phy2log(&_entry[from]);
                *pte = phy2pte(pte2phy(_entry[from]), flags);
            }
        }

        void unmap(int from, int to) {
            for( ; from < to; from++) {
                free(_entry[from]);
                Log_Addr * pte = phy2log(&_entry[from]);
                *pte = 0;
            }
        }

        friend OStream & operator<<(OStream & os, Page_Table & pt) {
            os << "{\n";
            int brk = 0;
            for(unsigned int i = 0; i < PT_ENTRIES; i++)
                if(pt[i]) {
                    os << "[" << i << "]=" << pte2phy(pt[i]) << "  ";
                    if(!(++brk % 4))
                        os << "\n";
                }
            os << "\n}";
            return os;
        }

    private:
        PT_Entry _entry[PT_ENTRIES]; // the Phy_Addr in each entry passed through phy2pte()
    };

    // Chunk (for Segment)
    class Chunk
    {
    public:
        Chunk(const Chunk & c): _free(false), _from(c._from), _to(c._to), _pts(c._pts), _flags(c._flags), _pt(c._pt) {} // avoid freeing memory when temporaries are created

        Chunk(unsigned long bytes, Flags flags, Color color = WHITE)
        : _free(true), _from(0), _to(pages(bytes)), _pts(Common::pts(_to - _from)), _flags(Page_Flags(flags)), _pt(calloc(_pts, WHITE)) {
            if(_flags & Page_Flags::CT)
                _pt->map_contiguous(_from, _to, _flags, color);
            else
                _pt->map(_from, _to, _flags, color);
        }

        Chunk(Phy_Addr phy_addr, unsigned long bytes, Flags flags)
        : _free(true), _from(0), _to(pages(bytes)), _pts(Common::pts(_to - _from)), _flags(Page_Flags(flags)), _pt(calloc(_pts, WHITE)) {
            _pt->remap(phy_addr, _from, _to, flags);
        }

        Chunk(Phy_Addr pt, unsigned int from, unsigned int to, Flags flags)
        : _free(false), _from(from), _to(to), _pts(Common::pts(_to - _from)), _flags(flags), _pt(pt) {}

        Chunk(Phy_Addr pt, unsigned int from, unsigned int to, Flags flags, Phy_Addr phy_addr)
        : _free(false), _from(from), _to(to), _pts(Common::pts(_to - _from)), _flags(flags), _pt(pt) {
            _pt->remap(phy_addr, _from, _to, flags);
        }

        ~Chunk() {
            if(_free) {
                if(!(_flags & Page_Flags::IO)) {
                    if(_flags & Page_Flags::CT)
                        free((*_pt)[_from], _to - _from);
                    else
                        for( ; _from < _to; _from++)
                            free((*_pt)[_from]);
                }
                free(_pt, _pts);
            }
        }

        unsigned int pts() const { return _pts; }
        Page_Flags flags() const { return _flags; }
        Page_Table * pt() const { return _pt; }
        unsigned long size() const { return (_to - _from) * sizeof(Page); }
        
        void reflag(Flags flags) {
            _flags = flags;
            _pt->reflag(_from, _to, _flags);
        }

        Phy_Addr phy_address() const {
            return (_flags & Page_Flags::CT) ? Phy_Addr(unflag((*_pt)[_from])) : Phy_Addr(false);
        }

        unsigned long resize(long amount) {
            if(_flags & Page_Flags::CT)
                return 0;

            if(amount > 0) {
                unsigned long pgs = pages(amount);

                Color color = colorful ? phy2color(_pt) : WHITE;

                unsigned long free_pgs = _pts * PT_ENTRIES - _to;
                if(free_pgs < pgs) { // resize _pt
                    unsigned long pts = _pts + Common::pts(pgs - free_pgs);
                    Page_Table * pt = calloc(pts, color);
                    memcpy(phy2log(pt), phy2log(_pt), _pts * sizeof(Page));
                    free(_pt, _pts);
                    _pt = pt;
                    _pts = pts;
                }

                _pt->map(_to, _to + pgs, _flags, color);
                _to += pgs;
            } else
                db<MMU>(WRN) << "MMU::Chunk::resize(amount=" << amount << "): segment shrinking not implemented!" << endl;

            return size();
        }

    private:
        bool _free;
        unsigned int _from;
        unsigned int _to;
        unsigned int _pts;
        Page_Flags _flags;
        Page_Table * _pt; // this is a physical address
    };

    // Page Directory
    typedef Page_Table Page_Directory;

    // Directory (for Address_Space)
    class Directory
    {
    public:
        Directory(const Directory & d): _free(false), _pd(d._pd) {} // avoid freeing memory when temporaries are created

        Directory(): _free(true), _pd(calloc(1, WHITE)) {
            for(unsigned int i = 0; i < PD_ENTRIES; i++)
                if(!((i >= pdi(APP_LOW)) && (i <= pdi(APP_HIGH))))
                    _pd->log()[i] = _master->log()[i];
        }

        Directory(Page_Directory * pd): _free(false), _pd(pd) {}

        ~Directory() { if(_free) free(_pd); }

        Phy_Addr pd() const { return _pd; }

        void activate() const { SV32_MMU::pd(_pd); }

        Log_Addr find(const Chunk & chunk) {
            for(unsigned int i = 0; i < PD_ENTRIES; i++)
                if(unflag(pde2phy(_pd->log()[i])) == unflag(chunk.pt()))
                    return (i << PD_SHIFT);
            return Log_Addr(false);
        }

        Log_Addr attach(const Chunk & chunk) {
            for(Log_Addr addr = align_segment(APP_LOW); addr < (APP_HIGH - chunk.size()); addr += sizeof(Big_Page))
                if(attachable(addr, chunk.pt(), chunk.pts(), chunk.flags()))
                    return attach(addr, chunk.pt(), chunk.pts(), chunk.flags());
            return Log_Addr(false);
        }

        Log_Addr attach(const Chunk & chunk, Log_Addr addr) {
            if(addr != align_segment(addr)) {
                db<MMU>(WRN) << "MMU::Directory::attach(chunk=" << &chunk << ",addr=" << addr << "): not attaching chunk to misaligned address!" << endl;
                return Log_Addr(false);
            }
            if((PD_ENTRIES - pdi(addr)) < pts(pages(chunk.size()))) {
                db<MMU>(WRN) << "MMU::Directory::attach(chunk=" << &chunk << ",addr=" << addr << "): attaching chunk of " << pts(pages(chunk.size())) << " PTs at PD[" << pdi(addr) <<"] would reach beyond the limit of the address space!" << endl;
                return Log_Addr(false);
            }
            if(!attachable(addr, chunk.pt(), chunk.pts(), chunk.flags()))
                return Log_Addr(false);
            return attach(addr, chunk.pt(), chunk.pts(), chunk.flags());
        }

        void detach(const Chunk & chunk) {
            Log_Addr addr = find(chunk);
            if(!addr)
                db<MMU>(WRN) << "MMU::Directory::detach(chunk=" << &chunk << ") [pt=" << chunk.pt() << "] failed!" << endl;
            detach(addr, chunk.pt(), chunk.pts());
        }

        void detach(const Chunk & chunk, Log_Addr addr) {
            if(!detach(addr, chunk.pt(), chunk.pts()))
                db<MMU>(WRN) << "MMU::Directory::detach(chunk=" << &chunk << ",addr=" << addr << ") [pt=" << chunk.pt() << "] failed!" << endl;
        }

        Phy_Addr physical(Log_Addr addr) {
            PD_Entry pde = (*_pd)[pdi(addr)];
            Page_Table * pt = static_cast<Page_Table *>(pde2phy(pde));
            PT_Entry pte = pt->log()[pti(addr)];
            return pte | off(addr);
        }

    private:
        bool attachable(Log_Addr addr, const Page_Table * pt, unsigned int pts, Page_Flags flags) {
            for(unsigned int i = pdi(addr); i < pdi(addr) + pts; i++) {
                if(pde2phy(_pd->log()[i]))
                    return false;
            }
            return true;
        }

        Log_Addr attach(Log_Addr addr, const Page_Table * pt, unsigned int pts, Page_Flags flags) {
            for(unsigned int i = pdi(addr); i < pdi(addr) + pts; i++, pt++)
                _pd->log()[i] = phy2pde(Phy_Addr(pt));
            return addr;
        }

        Log_Addr detach(Log_Addr addr, const Page_Table * pt, unsigned int pts) {
            for(unsigned int i = pdi(addr); i < pdi(addr) + pts; i++) {
                _pd->log()[i] = 0;
                flush_tlb(i << PD_SHIFT);
            }
            flush_tlb();
            return addr;
        }

    private:
        bool _free;
        Page_Directory * _pd;  // this is a physical address, but operator*() returns a logical address
    };

    // DMA_Buffer
    class DMA_Buffer: public Chunk
    {
    public:
        DMA_Buffer(unsigned long s): Chunk(s, Flags::DMA) {
            Directory dir(current());
            _log_addr = dir.attach(*this);
            db<MMU>(TRC) << "MMU::DMA_Buffer(s=" << s << ") => " << this << endl;
            db<MMU>(INF) << "MMU::DMA_Buffer=" << *this << endl;
        }

        DMA_Buffer(unsigned long s, Log_Addr d): Chunk(s, Flags::DMA) {
            Directory dir(current());
            _log_addr = dir.attach(*this);
            memcpy(_log_addr, d, s);
            db<MMU>(TRC) << "MMU::DMA_Buffer(s=" << s << ") => " << this << endl;
        }

        Log_Addr log_address() const { return _log_addr; }

        friend OStream & operator<<(OStream & os, const DMA_Buffer & b) {
            os << "{phy=" << b.phy_address() << ",log=" << b.log_address() << ",size=" << b.size() << ",flags=" << b.flags() << "}";
            return os;
        }

    private:
        Log_Addr _log_addr;
    };

    // Class Translation performs manual logical to physical address translations for debugging purposes only
    class Translation
    {
    public:
        Translation(Log_Addr addr, bool pt = false, Page_Directory * pd = 0): _addr(addr), _show_pt(pt), _pd(pd) {}

        friend OStream & operator<<(OStream & os, const Translation & t) {
            Page_Directory * pd = t._pd ? t._pd : current();
            PD_Entry pde = pd->log()[pdi(t._addr)];
            Page_Table * pt = static_cast<Page_Table *>(pde2phy(pde));
            PT_Entry pte = pt->log()[pti(t._addr)];

            os << "{addr=" << static_cast<void *>(t._addr) << ",pd=" << pd << ",pd[" << pdi(t._addr) << "]=" << pde << ",pt=" << pt;
            if(t._show_pt)
                os << "=>" << pt->log();
            os << ",pt[" << pti(t._addr) << "]=" << pte << ",f=" << pte2phy(pte) << ",*addr=" << hex << *static_cast<unsigned long *>(t._addr) << "}";
            return os;
        }

    private:
        Log_Addr _addr;
        bool _show_pt;
        Page_Directory * _pd;
    };

public:
    SV32_MMU() {}

    static Phy_Addr alloc(unsigned long frames = 1, Color color = WHITE) {
        Phy_Addr phy(false);

        if(frames) {
            List::Element * e = _free[color].search_decrementing(frames);
            if(e) {
                phy = e->object() + e->size();
                db<MMU>(TRC) << "MMU::alloc(frames=" << frames << ",color=" << color << ") => " << phy << endl;
            } else
                if(colorful)
                    db<MMU>(INF) << "MMU::alloc(frames=" << frames << ",color=" << color << ") => failed!" << endl;
                else
                    db<MMU>(WRN) << "MMU::alloc(frames=" << frames << ",color=" << color << ") => failed!" << endl;
        }

        return phy;
    }

    static Phy_Addr calloc(unsigned long frames = 1, Color color = WHITE) {
        Phy_Addr phy = alloc(frames, color);
        memset(phy2log(phy), 0, sizeof(Frame) * frames);
        return phy;
    }

    static void free(Phy_Addr frame, unsigned long n = 1) {
        // Clean up MMU flags in frame address
        frame = unflag(frame);
        Color color = colorful ? phy2color(frame) : WHITE;

        db<MMU>(TRC) << "MMU::free(frame=" << frame << ",color=" << color << ",n=" << n << ")" << endl;

        if(frame && n) {
            List::Element * e = new (phy2log(frame)) List::Element(frame, n);
            List::Element * m1, * m2;
            _free[color].insert_merging(e, &m1, &m2);
        }
    }

    static void white_free(Phy_Addr frame, unsigned long n) {
        // Clean up MMU flags in frame address
        frame = unflag(frame);

        db<MMU>(TRC) << "MMU::free(frame=" << frame << ",color=" << WHITE << ",n=" << n << ")" << endl;

        if(frame && n) {
            List::Element * e = new (phy2log(frame)) List::Element(frame, n);
            List::Element * m1, * m2;
            _free[WHITE].insert_merging(e, &m1, &m2);
        }
    }

    static unsigned long allocable(Color color = WHITE) { return _free[color].head() ? _free[color].head()->size() : 0; }

    static Page_Directory * volatile current() { return static_cast<Page_Directory * volatile>(pd()); }

    static Phy_Addr physical(Log_Addr addr) {
        Page_Directory * pd = current();
        Page_Table * pt = pd->log()[pdi(addr)];
        return pt->log()[pti(addr)] | off(addr);
    }

    static PT_Entry phy2pte(Phy_Addr frame, Page_Flags flags) { return (frame >> 2) | flags; }
    static Phy_Addr pte2phy(PT_Entry entry) { return (entry & ~Page_Flags::MASK) << 2; }
    static PD_Entry phy2pde(Phy_Addr frame) { return (frame >> 2) | Page_Flags::V; }
    static Phy_Addr pde2phy(PD_Entry entry) { return (entry & ~Page_Flags::MASK) << 2; }

#ifdef __setup__
    // SETUP may use the MMU to build a primordial memory model before turning the MMU on, so no log vs phy adjustments are made
    static Log_Addr phy2log(Phy_Addr phy) { return Log_Addr((RAM_BASE == PHY_MEM) ? phy : (RAM_BASE > PHY_MEM) ? phy : phy ); }
    static Phy_Addr log2phy(Log_Addr log) { return Phy_Addr((RAM_BASE == PHY_MEM) ? log : (RAM_BASE > PHY_MEM) ? log : log ); }
#else
    static Log_Addr phy2log(Phy_Addr phy) { return Log_Addr((RAM_BASE == PHY_MEM) ? phy : (RAM_BASE > PHY_MEM) ? phy - (RAM_BASE - PHY_MEM) : phy + (PHY_MEM - RAM_BASE)); }
    static Phy_Addr log2phy(Log_Addr log) { return Phy_Addr((RAM_BASE == PHY_MEM) ? log : (RAM_BASE > PHY_MEM) ? log + (RAM_BASE - PHY_MEM) : log - (PHY_MEM - RAM_BASE)); }
#endif

    static Color phy2color(Phy_Addr phy) { return static_cast<Color>(colorful ? ((phy >> PT_SHIFT) & 0x7f) % COLORS : WHITE); } // TODO: what is 0x7f

    static Color log2color(Log_Addr log) {
        if(colorful) {
            Page_Directory * pd = current();
            Page_Table * pt = pd->log()[pdi(log)];
            Phy_Addr phy = pt->log()[pti(log)] | off(log);
            return static_cast<Color>(((phy >> PT_SHIFT) & 0x7f) % COLORS);
        } else
            return WHITE;
    }

private:
    static Phy_Addr pd() { return CPU::satp() << PT_SHIFT; }
    static void pd(Phy_Addr pd) { CPU::satp((1UL << 31) | (pd >> PT_SHIFT)); }

    static void flush_tlb() { CPU::flush_tlb(); }
    static void flush_tlb(Log_Addr addr) { CPU::flush_tlb(addr); }

    static void init();

private:
    static List _free[colorful * COLORS + 1]; // +1 for WHITE
    static Page_Directory * _master;
};

class MMU: public No_MMU {};

__END_SYS

#endif
