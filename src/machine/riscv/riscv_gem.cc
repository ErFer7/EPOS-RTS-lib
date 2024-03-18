#include <system/config.h>

#include <machine/machine.h>
#include <machine/riscv/riscv_gem.h>
#include <system.h>
#include <time.h>

__BEGIN_SYS

GEM::Device GEM::_device;


GEM::~GEM()
{
    db<GEM>(TRC) << "~GEM()" << endl;
}

int GEM::send(const Address & dst, const Protocol & prot, const void * data, unsigned int size)
{
    // _tx_cur and _rx_cur are simple accelerators to avoid scanning the ring buffer from the beginning.
    // Losing a write in a race condition is assumed to be harmless. The FINC + CAS alternative seems too expensive.

    db<GEM>(TRC) << "GEM::send(s=" << _configuration.address << ",d=" << dst << ",p=" << hex << prot << dec << ",d=" << data << ",s=" << size << ")" << endl;

    // Wait for a buffer to become free and seize it
    unsigned long i = _tx_cur;
    for(bool locked = false; !locked;) {
        for(; !(_tx_ring[i].ctrl & Tx_Desc::OWN); ++i %= TX_BUFS);
        locked = _tx_buffer[i]->lock();
    }
    _tx_cur = (i + 1) % TX_BUFS;

    db<GEM>(INF) << "GEM::send:buf=" << _tx_buffer[i] << " => " << *_tx_buffer[i] << endl;

    Tx_Desc * desc = &_tx_ring[i];
    Buffer * buf = _tx_buffer[i];

    // Give the NIC ownership of the buffer (so it can send it)
    desc->ctrl &= ~Tx_Desc::OWN;

    // Assemble the Ethernet frame
    new (buf->frame()) Frame(_configuration.address, dst, prot, data, size);

    desc->size(size + sizeof(Header));

    desc->ctrl |= Tx_Desc::LAST_BUF;

    db<GEM>(INF) << "GEM::send:before_desc[" << i << "]=" << desc << " => " << *desc << endl;

    // Immediately start transmitting
    start_tx();

    _statistics.tx_packets++;
    _statistics.tx_bytes += size;

    db<GEM>(INF) << "GEM::send:after_desc[" << i << "]=" << desc << " => " << *desc << endl;

    buf->unlock();

    return size;
}

int GEM::receive(Address * src, Protocol * prot, void * data, unsigned int size)
{
    db<GEM>(TRC) << "GEM::receive(s=" << *src << ",p=" << hex << *prot << dec << ",d=" << data << ",s=" << size << ") => " << endl;

    // Wait for a received frame and seize it
    unsigned int i = _rx_cur;
    for(bool locked = false; !locked;) {
        for(; !(_rx_ring[i].addr & Rx_Desc::OWN); ++i %= RX_BUFS);
        locked = _rx_buffer[i]->lock();
    }
    _rx_cur = (i + 1) % RX_BUFS;
    Buffer * buf = _rx_buffer[i];
    Rx_Desc * desc = &_rx_ring[i];

    // Disassemble the Ethernet frame
    Frame* frame = buf->frame();
    *src = frame->src();
    *prot = frame->prot();

    // For the upper layers, size will represent the size of frame->data<T>()
    buf->size((desc->ctrl & Rx_Desc::SIZE_MASK) - sizeof(Header));

    // Copy the data
    memcpy(data, frame->data<void>(), (buf->size() > size) ? size : buf->size());

    // Release the buffer to the NIC
    desc->addr &= ~Rx_Desc::OWN;
    desc->ctrl = 0;

    _statistics.rx_packets++;
    _statistics.rx_bytes += buf->size();

    db<GEM>(INF) << "GEM::receive:desc[" << i << "]=" << desc << " => " << *desc << endl;

    int tmp = buf->size();

    buf->unlock();

    return tmp;
}

GEM::Buffer * GEM::alloc(const Address& dst, const Protocol& prot, unsigned int once, unsigned int always, unsigned int payload)
{
    db<GEM>(TRC) << "GEM::alloc(s=" << _configuration.address << ",d=" << dst << ",p=" << hex << prot << dec << ",on=" << once << ",al=" << always << ",pl=" << payload << ")" << endl;

    int max_data = MTU - always;

    if((payload + once) / max_data > TX_BUFS) {
        db<GEM>(WRN) << "GEM::alloc: sizeof(Network::Packet::Data) > sizeof(NIC::Frame::Data) * TX_BUFS!" << endl;
        return 0;
    }

    Buffer::List pool;

    // Calculate how many frames are needed to hold the transport PDU and allocate enough buffers
    for(int size = once + payload; size > 0; size -= max_data) {
        // Wait for the next buffer to become free and seize it
        unsigned int i = _tx_cur;
        for(bool locked = false; !locked;) {
            for(; !(_tx_ring[i].ctrl & Tx_Desc::OWN); ++i %= TX_BUFS);
            locked = _tx_buffer[i]->lock();
        }
        _tx_cur = (i + 1) % TX_BUFS;
        Tx_Desc * desc = &_tx_ring[i];
        Buffer * buf = _tx_buffer[i];

        // Initialize the buffer and assemble the Ethernet Frame Header
        buf->fill((size > max_data) ? MTU : size + always, _configuration.address, dst, prot);

        db<GEM>(INF) << "GEM::alloc:desc[" << i << "]=" << desc << " => " << *desc << endl;
        db<GEM>(INF) << "GEM::alloc:buf=" << buf << " => " << *buf << endl;

        pool.insert(buf->link());
    }

    return pool.head()->object();
}

int GEM::send(Buffer * buf)
{
    unsigned int size = 0;

    for(Buffer::Element* el = buf->link(); el; el = el->next()) {
        buf = el->object();
        Tx_Desc * desc = reinterpret_cast<Tx_Desc*>(buf->back());

        db<GEM>(INF) << "GEM::send:buf=" << buf << " => " << *buf << endl;

        desc->size(buf->size() + sizeof(Header));

        // Give the NIC ownership of the buffer (so it can send it)
        desc->ctrl &= ~Tx_Desc::OWN;

        // Immediately start transmitting
        start_tx();

        size += buf->size();

        _statistics.tx_packets++;
        _statistics.tx_bytes += buf->size();

        db<GEM>(INF) << "GEM::send:desc=" << desc << " => " << *desc << endl;

        // Wait for packet to be sent and release it
        while (!(desc->ctrl & Tx_Desc::OWN));
        buf->unlock();
    }

    return size;
}

void GEM::free(Buffer* buf)
{
    db<GEM>(TRC) << "GEM::free(buf=" << buf << ")" << endl;

    db<GEM>(INF) << "GEM::free:buf=" << buf << " => " << *buf << endl;

    for(Buffer::Element* el = buf->link(); el; el = el->next()) {
        buf = el->object();
        Rx_Desc* desc = reinterpret_cast<Rx_Desc*>(buf->back());

        _statistics.rx_packets++;
        _statistics.rx_bytes += buf->size();

        // Release the buffer to the NIC
        desc->size(sizeof(Frame));
        desc->ctrl &= ~Rx_Desc::OWN; // Owned by NIC

        // Release the buffer to the OS
        buf->unlock();

        db<GEM>(INF) << "GEM::free:desc=" << desc << " => " << *desc << endl;
    }
}

void GEM::receive()
{
    TSC::Time_Stamp ts =
        (Buffer::Metadata::collect_sfdts) ? TSC::time_stamp() : 0;

    db<GEM>(TRC) << "GEM::receive()" << endl;

    for (unsigned int count = RX_BUFS, i = _rx_cur; count && ((_rx_ring[i].addr & Rx_Desc::OWN) != 0); count--, ++i %= RX_BUFS, _rx_cur = i) {
        // NIC received a frame in _rx_buffer[_rx_cur], let's check if it has already been handled
        if (_rx_buffer[i]->lock()) { // if it wasn't, let's handle it
            Buffer* buf = _rx_buffer[i];
            Rx_Desc* desc = &_rx_ring[i];
            Frame* frame = buf->frame();

            desc->addr &= ~Rx_Desc::OWN; // Owned by NIC

            // For the upper layers, size will represent the size of frame->data<T>()
            buf->size((desc->ctrl & Rx_Desc::SIZE_MASK) - sizeof(Header));

            db<GEM>(TRC) << "GEM::receive: frame = " << *frame << endl;
            db<GEM>(INF) << "GEM::handle_int: desc[" << i << "] = " << desc << " => " << *desc << endl;

            if((Traits<GEM>::EXPECTED_SIMULATION_TIME > 0) && (frame->header()->src() == _configuration.address)) { // Multicast on QEMU seems to replicate all sent packets
                free(buf);
                continue;
            }

            if (Buffer::Metadata::collect_sfdts)
                buf->sfdts = ts;
            if (!notify(frame->header()->prot(), buf)) // No one was waiting for this frame, so let it free for receive()
                free(buf);
        }
    }
}

bool GEM::reconfigure(const Configuration* c = 0)
{
    db<GEM>(TRC) << "GEM::reconfigure(c=" << c << ")" << endl;

    bool ret = false;

    if(!c) {
        db<GEM>(TRC) << "GEM::reconfigure: resetting!" << endl;
        CPU::int_disable();
        configure(_configuration.address, _tx_ring_phy, _rx_ring_phy);
        new (&_statistics) Statistics; // reset statistics
        CPU::int_enable();
    } else {
        db<GEM>(INF) << "GEM::reconfigure: configuration = " << *c << ")" << endl;

        if(c->selector & Configuration::ADDRESS) {
            db<GEM>(WRN) << "GEM::reconfigure: address changed only in the mediator!)" << endl;
            _configuration.address = c->address;
            ret = true;
        }

        if(c->selector & Configuration::TIMER) {
            if(c->timer_frequency) {
                db<GEM>(WRN) << "GEM::reconfigure: timer frequency cannot be changed!)" << endl;
                ret = false;
            }
        }
    }

    return ret;
}

void GEM::handle_int()
{
    Reg32 status = isr();
    clear_isr();

    db<GEM>(TRC) << "GEM::handle_int: status=" << hex << status << endl;

    if((status & INTR_RX_COMPLETE)) {
        db<GEM>(TRC) << "GEM::handle_int: RX_CMPL" << endl;

        IC::disable(IC::INT_ETH0);
        complete_rx();
        receive();
        IC::enable(IC::INT_ETH0);
    }

    if((status & INTR_TX_COMPLETE)) {
        complete_tx();

        unsigned int i = _tx_cur == 0 ? TX_BUFS - 1 : _tx_cur - 1;

        db<GEM>(INF) << "GEM::handle_int: Unlocking _tx_buffer[" << i << "] => " << _tx_ring[i] << endl;

        _tx_ring[i].ctrl = _tx_ring[i].ctrl & (Tx_Desc::OWN | Tx_Desc::WRAP); // Keep OWN and WRAP bits
        _tx_buffer[i]->unlock();
    }

    if(status & INTR_TX_USED_READ) {
        db<GEM>(INF) << "GEM::handle_int: out of TX buffers" << endl;
        tx_used_read_write();
    }

    if(status & INTR_RX_USED_READ) {
        db<GEM>(INF) << "GEM::handle_int: out of RX buffers" << endl;
        rx_used_read_write();
    }

    if(status & INTR_RX_OVERRUN) { // missed Frame
        db<GEM>(INF) << "GEM::handle_int: error => missed frame" << endl;
        rx_overrun_write();
        _statistics.rx_overruns++;
    }

    if(status & INTR_TX_CORRUPT_AHB_ERR) // bus error
        db<GEM>(WRN) << "GEM::handle_int: error => TX_CORRUPT_AHB_ERR" << endl;

    if(status & INTR_TX_UNDERRUN)
        db<GEM>(INF) << "GEM::handle_int: error => TX_UNDERRUN" << endl;
}

void GEM::int_handler(unsigned int interrupt)
{
    GEM * dev = _device.device;

    db<GEM, IC>(TRC) << "GEM::int_handler(int=" << interrupt << ",dev=" << dev << ")" << endl;

    if(!dev)
        db<GEM>(WRN) << "GEM::int_handler: handler not installed!\n";
    else
        dev->handle_int();
}

__END_SYS