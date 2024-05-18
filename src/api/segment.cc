// EPOS Memory Segment Implementation

#include <memory.h>
#include <process.h>

__BEGIN_SYS

// Methods
Segment::Segment(unsigned long bytes, Flags flags): Chunk(bytes, flags, WHITE)
{
    db<Segment>(TRC) << "Segment(bytes=" << bytes << ",flags=" << flags << ") [Chunk::pt=" << Chunk::pt() << ",sz=" << Chunk::size() << "] => " << this << endl;

    if(Task::self()) // segments can be created at boot-time, before Task::init()
        Task::self()->enroll(this);
}


Segment::Segment(Phy_Addr phy_addr, unsigned long bytes, Flags flags): Chunk(phy_addr, bytes, flags | Flags::IO)
// The MMU::IO flag signalizes the MMU that the attached memory shall not be released when the chunk is deleted
{
    db<Segment>(TRC) << "Segment(bytes=" << bytes << ",phy_addr=" << phy_addr << ",flags=" << flags << ") [Chunk::pt=" << Chunk::pt() << ",sz=" << Chunk::size() << "] => " << this << endl;
}


Segment::~Segment()
{
    db<Segment>(TRC) << "~Segment() [Chunk::pt=" << Chunk::pt() << "]" << endl;
    
    if(Task::self()) // segments can be created at boot-time, before Task::init()
        Task::self()->dismiss(this);
}


unsigned long Segment::size() const
{
    return Chunk::size();
}


Segment::Phy_Addr Segment::phy_address() const
{
    return Chunk::phy_address();
}


long Segment::resize(long amount)
{
    db<Segment>(TRC) << "Segment::resize(amount=" << amount << ")" << endl;

    return Chunk::resize(amount);
}


void Segment::reflag(Flags flags)
{
    Chunk::reflag(flags);
}

__END_SYS
