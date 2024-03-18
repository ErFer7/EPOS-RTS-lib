// EPOS Ethernet Mediator Common Package

#ifndef __ethernet_h
#define __ethernet_h

#include <architecture/cpu.h>
#define __nic_common_only__
#include <machine/nic.h>
#undef __nic_common_only__
#include <utility/list.h>
#include <utility/observer.h>
#include <utility/buffer.h>

__BEGIN_SYS

class Ethernet: public NIC_Common
{
protected:
    static const unsigned int HEADER_SIZE = 14;

public:
    typedef NIC_Common::Address<6> Address;

    typedef unsigned short Protocol;
    enum {
        PROTO_IP     = 0x0800,
        PROTO_ARP    = 0x0806,
        PROTO_RARP   = 0x8035,
        PROTO_TSTP   = 0x8401,
        PROTO_ELP    = 0x8402,
        PROTO_PTP    = 0x88f7
    };

    // The Ethernet Header (RFC 894)
    class Header
    {
    public:
        Header() {}
        Header(const Address & src, const Address & dst, const Protocol & prot): _dst(dst), _src(src), _prot(htons(prot)) {}

        const Address & src() const { return _src; }
        const Address & dst() const { return _dst; }

        Protocol prot() const { return ntohs(_prot); }

        friend Debug & operator<<(Debug & db, const Header & h) {
            db << "{d=" << h._dst << ",s=" << h._src << ",p=" << hex << h.prot() << dec << "}";
            return db;
        }

    protected:
        Address _dst;
        Address _src;
        Protocol _prot;
    } __attribute__((packed, may_alias));

    // Data and Trailer
    static const unsigned int MTU = 1500;
    typedef unsigned char Data[MTU];

    typedef NIC_Common::CRC32 CRC;
    typedef CRC Trailer;

    // The Ethernet Frame (RFC 894)
    class Frame: public Header
    {
    public:
        Frame() {}
        Frame(const Address & src, const Address & dst, const Protocol & prot): Header(src, dst, prot) {}
        Frame(const Address & src, const Address & dst, const Protocol & prot, const void * data, unsigned int size): Header(src, dst, prot) { memcpy(_data, data, size); }

        Header * header() { return this; }

        template<typename T>
        T * data() { return reinterpret_cast<T *>(&_data); }

        friend Debug & operator<<(Debug & db, const Frame & f) {
            db << "{h=" << reinterpret_cast<const Header &>(f) << ",d=" << f._data << "}";
            return db;
        }

    protected:
        Data _data;
        CRC _crc;
    } __attribute__((packed));

    typedef Frame PDU;

    typedef IF<Traits<TSTP>::enabled, TSTP_Metadata, Dummy_Metadata>::Result Metadata;

    // Buffers used to hold frames across a zero-copy network stack
    typedef _UTIL::Buffer<NIC<Ethernet>, Frame, void, Metadata> Buffer;

    // Observers of a protocol get a also a pointer to the received buffer
    typedef Data_Observer<Buffer, Protocol> Observer;
    typedef Data_Observed<Buffer, Protocol> Observed;

    // Ethernet NICs usually don't export the timer for SFD time stamping, so the basic time type is set to TSC
    // NICs that do feature time stamping must have any different type converted to TSC::Time_Stamp
    typedef TSC::Time_Stamp Time_Stamp;

    // Configuration parameters
    struct Configuration: public NIC_Common::Configuration
    {
        friend Debug & operator<<(Debug & db, const Configuration & c) {
            db << "{unit=" << c.unit
               << ",addr=" << c.address
               << ",a=" << c.timer_accuracy
               << ",f=" << c.timer_frequency
               << "}";
            return db;
        }

        unsigned int unit;
        Address address;
        PPM timer_accuracy;
        Hertz timer_frequency;
    };

    // Meaningful statistics for Ethernet
    struct Statistics: public NIC_Common::Statistics
    {
        Statistics(): rx_overruns(0), tx_overruns(0), frame_errors(0), carrier_errors(0), collisions(0) {}

        friend Debug & operator<<(Debug & db, const Statistics & s) {
            db << "{rxp=" << s.rx_packets
               << ",rxb=" <<  s.rx_bytes
               << ",rxorun=" <<  s.rx_overruns
               << ",txp=" <<  s.tx_packets
               << ",txb=" <<  s.tx_bytes
               << ",txorun=" <<  s.tx_overruns
               << ",frm=" <<  s.frame_errors
               << ",car=" <<  s.carrier_errors
               << ",col=" <<  s.collisions
               << "}";
            return db;
        }

        Time_Stamp time_stamp;
        Count rx_overruns;
        Count tx_overruns;
        Count frame_errors;
        Count carrier_errors;
        Count collisions;
    };

protected:
    Ethernet() {}

public:
    static const unsigned int mtu() { return MTU; }
    static const Address broadcast() { return Address::BROADCAST; }
};

__END_SYS

#endif
