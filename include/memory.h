// EPOS Memory Declarations

#ifndef __memory_h
#define __memory_h

#include <architecture.h>

__BEGIN_SYS

class Address_Space: private MMU::Directory
{
public:
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;

public:
    Address_Space();
    Address_Space(MMU::Page_Directory * pd);
    ~Address_Space();

    using MMU::Directory::pd;

    Log_Addr attach(Segment * seg);
    Log_Addr attach(Segment * seg, Log_Addr addr);
    void detach(Segment * seg);
    void detach(Segment * seg, Log_Addr addr);

    Phy_Addr physical(Log_Addr address);
};


class Segment: public MMU::Chunk
{
private:
    typedef MMU::Chunk Chunk;

public:
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;
    typedef MMU::Flags Flags;

public:
    Segment(unsigned long bytes, Flags flags = Flags::APPD);
    Segment(Phy_Addr phy_addr, unsigned long bytes, Flags flags);
    ~Segment();

    unsigned long size() const;
    Phy_Addr phy_address() const;
    long resize(long amount);
    void reflag(Flags flags);
};

__END_SYS

#endif
