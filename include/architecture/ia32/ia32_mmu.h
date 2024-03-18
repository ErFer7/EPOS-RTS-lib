// EPOS IA32 MMU Mediator Declarations

#ifndef __ia32_mmu_h
#define __ia32_mmu_h

#include <architecture/mmu.h>
#include <system/memory_map.h>

__BEGIN_SYS

class MMU: public MMU_Common<10, 10, 12>
{
    friend class CPU;
    friend class Setup;

private:
    typedef Grouping_List<Frame> List;
    typedef MMU_Common<10, 10, 12> Common;

    static const bool colorful = Traits<MMU>::colorful;
    static const unsigned int COLORS = Traits<MMU>::COLORS;
    static const unsigned int RAM_BASE  = Memory_Map::RAM_BASE;
    static const unsigned int APP_LOW   = Memory_Map::APP_LOW;
    static const unsigned int APP_HIGH  = Memory_Map::APP_HIGH;
    static const unsigned int PHY_MEM   = Memory_Map::PHY_MEM;
    static const unsigned int SYS       = Memory_Map::SYS;
    static const unsigned int SYS_HIGH  = Memory_Map::SYS_HIGH;

public:
    // Page Flags
    class Page_Flags
    {
    public:
        enum : unsigned long {
            PRE  = 1 <<  0, // Present (0=not-present, 1=present)
            WR   = 1 <<  1, // Writable (0=read-only, 1=read-write)
            USR  = 1 <<  2, // Access Control (0=supervisor, 1=user)
            PWT  = 1 <<  3, // Cache mode (0=write-back, 1=write-through)
            PCD  = 1 <<  4, // Cache disable (0=cacheable, 1=non-cacheable)
            ACC  = 1 <<  5, // Accessed (0=not-accessed, 1=accessed
            DRT  = 1 <<  6, // Dirty (only for PTEs, 0=clean, 1=dirty)
            PS   = 1 <<  7, // Page Size (for PDEs, 0=4KBytes, 1=4MBytes)
            GLB  = 1 <<  8, // Global Page (0=local, 1=global)
            EX   = 1 <<  9, // User Def. (0=non-executable, 1=executable)
            CT   = 1 << 10, // User Def. (0=non-contiguous, 1=contiguous)
            IO   = 1 << 11, // User Def. (0=memory, 1=I/O)
            APP  = (PRE | WR  | ACC | USR),
            APPC = (PRE | EX  | ACC | USR),
            APPD = (PRE | WR  | ACC | USR),
            SYS  = (PRE | WR  | ACC),
            PCI  = (SYS | PCD | IO),
            APIC = (SYS | PCD),
            VGA  = (SYS | PCD),
            DMA  = (SYS | PCD | CT),
            MASK = (1 << 12) - 1
        };

    public:
        Page_Flags() {}
        Page_Flags(unsigned long f) : _flags(f) {}
        Page_Flags(Flags f) : _flags(PRE | ACC |
                                    ((f & Flags::WR)  ? WR  : 0) |
                                    ((f & Flags::USR) ? USR : 0) |
                                    ((f & Flags::CWT) ? PWT : 0) |
                                    ((f & Flags::CD)  ? PCD : 0) |
                                    ((f & Flags::CT)  ? CT  : 0) |
                                    ((f & Flags::IO)  ? PCI : 0) ) {}

        operator unsigned long() const { return _flags; }

        friend OStream & operator<<(OStream & os, const Page_Flags & f) { os << hex << f._flags; return os; }

    private:
        unsigned long _flags;
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

    // Page Directory
    typedef Page_Table Page_Directory;

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

        void activate() const { MMU::pd(_pd); }

        Log_Addr attach(const Chunk & chunk, unsigned int from = pdi(APP_LOW)) {
            for(unsigned int i = from; (i + chunk.pts()) <= pdi(APP_HIGH); i++)
                if(attach(i, chunk.pt(), chunk.pts(), chunk.flags()))
                    return i << PD_SHIFT;
            return Log_Addr(false);
        }

        Log_Addr attach(const Chunk & chunk, Log_Addr addr) {
            unsigned int from = pdi(addr);
            if((from + chunk.pts()) > PD_ENTRIES)
                return Log_Addr(false);
            if(attach(from, chunk.pt(), chunk.pts(), chunk.flags()))
                return from << PD_SHIFT;
            return Log_Addr(false);
        }

        void detach(const Chunk & chunk) {
            for(unsigned int i = 0; i < PD_ENTRIES; i++) {
                if(unflag(pte2phy((*_pd)[i])) == unflag(chunk.pt())) {
                    detach(i, chunk.pt(), chunk.pts());
                    return;
                }
            }
            db<MMU>(WRN) << "MMU::Directory::detach(pt=" << chunk.pt() << ") failed!" << endl;
        }

        void detach(const Chunk & chunk, Log_Addr addr) {
            unsigned int from = pdi(addr);
            if(unflag(pte2phy((*_pd)[from])) != unflag(chunk.pt())) {
                db<MMU>(WRN) << "MMU::Directory::detach(pt=" << chunk.pt() << ",addr=" << addr << ") failed!" << endl;
                return;
            }
            detach(from, chunk.pt(), chunk.pts());
        }

        Phy_Addr physical(Log_Addr addr) {
            PD_Entry pde = (*_pd)[pdi(addr)];
            Page_Table * pt = static_cast<Page_Table *>(pde2phy(pde));
            PT_Entry pte = pt->log()[pti(addr)];
            return pte | off(addr);
        }

    private:
        bool attach(unsigned int from, const Page_Table * pt, unsigned int n, Page_Flags flags) {
            for(unsigned int i = from; i < from + n; i++)
                if(_pd->log()[i])
                    return false;
            for(unsigned int i = from; i < from + n; i++, pt++)
                _pd->log()[i] = phy2pde(Phy_Addr(pt), flags);
            return true;
        }

        void detach(unsigned int from, const Page_Table * pt, unsigned int n) {
            for(unsigned int i = from; i < from + n; i++) {
                _pd->log()[i] = 0;
                flush_tlb(i << PD_SHIFT);
            }
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
    MMU() {}

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

    static PT_Entry phy2pte(Phy_Addr frame, Page_Flags flags) { return frame | flags; }
    static Phy_Addr pte2phy(PT_Entry entry) { return (entry & ~Page_Flags::MASK); }
    static Page_Flags pte2flg(PT_Entry entry) { return (entry & Page_Flags::MASK); }
    static PD_Entry phy2pde(Phy_Addr frame, Page_Flags flags) { return frame | flags; }
    static Phy_Addr pde2phy(PD_Entry entry) { return (entry & ~Page_Flags::MASK); }
    static Page_Flags pde2flg(PT_Entry entry) { return (entry & Page_Flags::MASK); }

#ifdef __setup__
    // SETUP uses the MMU to build a primordial memory model before turning the MMU on, so no log vs phy adjustments are made
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
    static Phy_Addr pd() { return CPU::pd(); }
    static void pd(Phy_Addr pd) { CPU::pd(pd); }

    static void flush_tlb() { CPU::flush_tlb(); }
    static void flush_tlb(Log_Addr addr) { CPU::flush_tlb(addr); }

    static void init();

private:
    static List _free[colorful * COLORS + 1]; // +1 for WHITE
    static Page_Directory * _master;
};

__END_SYS

#endif
