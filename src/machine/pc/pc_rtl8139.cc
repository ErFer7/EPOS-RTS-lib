/*
* EPOS RTL8139 PC Ethernet NIC Mediator Initialization
* Authors: Gabriel Müller, Arthur Bridi Guazzelli and Juliana Silva Pinheiro
* Year: 2019
*/
#include <machine/machine.h>
#include <machine/pc/pc_rtl8139.h>
#include <process.h>
#include <system.h>
#include <time.h>

__BEGIN_SYS

RTL8139::Device RTL8139::_devices[UNITS];


RTL8139::~RTL8139()
{
    db<RTL8139>(TRC) << "~RTL8139()" << endl;
}

int RTL8139::send(const Address & dst, const Protocol & prot, const void * data, unsigned int size)
{
    db<RTL8139>(TRC) << "RTL8139 will send, _tx_cur " << _tx_cur << endl;

    unsigned int i = _tx_cur;
    for(bool locked = false; !locked; ) {
        for(; !(CPU::in32(_io_port + TRSTATUS + i * TX_BUFS) & OWN); ++i %= TX_BUFS);
        locked = _tx_buffer[i]->lock();
    }
    _tx_cur = (i + 1) % TX_BUFS;

    Buffer * buf = _tx_buffer[i];

    db<RTL8139>(TRC) << "RTL8139::send(s=" << _configuration.address << ",d=" << dst << ",p=" << hex << prot << dec << ",d=" << data << ",s=" << size << ")" << endl;

    new (buf->frame()) Frame(_configuration.address, dst, prot, data, size);
    unsigned short status = sizeof(Frame) & 0xfff; // write 0 on OWN bit

    CPU::out32(_io_port + TRSTART  + i * TX_BUFS, (long unsigned int) _tx_base_phy[i]);
    CPU::out32(_io_port + TRSTATUS + i * TX_BUFS, status);

    _statistics.tx_packets++;
    _statistics.tx_bytes += size;

    db<RTL8139>(TRC) << "RTL8139 sent, _tx_cur now at " << _tx_cur << endl;

    return size;
}

int RTL8139::receive(Address * src, Protocol * prot, void * data, unsigned int size)
{
    db<RTL8139>(TRC) << "RTL8139::receive(s=" << *src << ",p=" << hex << *prot << dec << ",d=" << data << ",s=" << size << ") => " << endl;

    while (!(CPU::in16(_io_port + ISR) & ROK) // while not received a packet
            &&
            !( // or not pending packets
            (CPU::in16(_io_port + CBR) < _rx_read) || 
            (_rx_read + sizeof(Frame)) < CPU::in16(_io_port + CBR)
            )
        );
    CPU::out16(_io_port + ISR, ROK);
    unsigned int * rx = (unsigned int *) (_rx_buffer + _rx_read);
    db<RTL8139>(TRC) << "rx=" << rx << endl;
    unsigned int header = *rx;
    db<RTL8139>(TRC) << "header=" << hex << header << endl;
    unsigned int packet_len = (header >> 16);
    unsigned int status = header & 0xffff;
    rx++;

    db<RTL8139>(TRC) << "packet_len=" << packet_len << endl;
    db<RTL8139>(TRC) << "status=" << status << endl;

    Frame * frame = reinterpret_cast<Frame *>(rx);
    db<RTL8139>(TRC) << "frame src " << frame->src() << endl;
    db<RTL8139>(TRC) << "frame d= " << *frame->data<char>() << endl;

    unsigned int size_packet = packet_len - sizeof(Header) - sizeof(CRC);

    // copy data to application
    memcpy(data, frame->data<char>(), (size_packet > size) ? size : size_packet);
    db<RTL8139>(TRC) << "buffer src " << frame->src() << endl;

    // update CAPR
    _rx_read += packet_len + TX_BUFS;
    _rx_read =  (_rx_read + 3) & ~3;

    if(_rx_read > RX_NO_WRAP_SIZE) {
        _rx_read -= RX_NO_WRAP_SIZE;
        db<RTL8139>(TRC) << "\tWRAPPED " << _rx_read << endl;
    }

    _statistics.rx_packets++;
    _statistics.rx_bytes += size_packet;

    CPU::out16(_io_port + CAPR, _rx_read - 0x10);
    db<RTL8139>(TRC) << "out(CAPR, " << _rx_read - 0x10<< ")" << endl;
    db<RTL8139>(TRC) << "CBR " << CPU::in16(_io_port + CBR) << endl;

    return size_packet;

}

RTL8139::Buffer * RTL8139::alloc(const Address & dst, const Protocol & prot, unsigned int once, unsigned int always, unsigned int payload)
{
    db<RTL8139>(TRC) << "RTL8139::alloc(on=" << once << ",al=" << always << ",pl=" << payload << ")" << endl;

    int max_data = MTU - always;

    if((payload + once) / max_data > TX_BUFS) {
        db<RTL8139>(WRN) << "RTL8139::alloc: sizeof(Network::Packet::Data) > sizeof(NIC::Frame::Data) * TX_BUFS!" << endl;
        return 0;
    }

    Buffer::List pool;

    // Calculate how many frames are needed to hold the transport PDU and allocate enough buffers
    for(int size = once + payload; size > 0; size -= max_data) {
        // Wait for the next buffer to become free and seize it
        unsigned int i = _tx_cur;
        for(bool locked = false; !locked; ) {
            for(; !(CPU::in32(_io_port + TRSTATUS + i * TX_BUFS) & OWN); ++i %= TX_BUFS);
            locked = _tx_buffer[i]->lock();
        }
        _tx_cur = (i + 1) % TX_BUFS;
        Buffer * buf = _tx_buffer[i];

        // Initialize the buffer and assemble the Ethernet Frame Header
        buf->fill((size > max_data) ? MTU : size + always, _configuration.address, dst, prot);

        db<RTL8139>(INF) << "RTL8139::alloc:buf=" << buf << " => " << *buf << endl;

        pool.insert(buf->link());
    }

    return pool.head()->object();
}

int RTL8139::send(Buffer * buf)
{
    unsigned int size = 0;

    for(Buffer::Element * el = buf->link(); el; el = el->next()) {
        buf = el->object();
        char * desc = reinterpret_cast<char *>(*reinterpret_cast<int *>(buf->back()));
        unsigned int i = 0;
        for (; i < TX_BUFS; i++)
            if(desc == _tx_base_phy[i]) break;

        db<RTL8139>(TRC) << "RTL8139::send(buf=" << buf << ",desc=" << desc << ",tx=" << _tx_base_phy[i] << ",i=" << i << ")" << endl;

        unsigned short status = sizeof(Frame) & 0xfff; // write 0 on OWN bit

        CPU::out32(_io_port + TRSTART  + i * TX_BUFS, (long unsigned int) _tx_base_phy[i]);
        CPU::out32(_io_port + TRSTATUS + i * TX_BUFS, status);

        size += buf->size();

        _statistics.tx_packets++;
        _statistics.tx_bytes += buf->size();

        db<RTL8139>(INF) << "RTL8139::send:desc=" << desc << " => " << *desc << endl;

        // Wait for packet to be sent and unlock the respective buffer
        while(!(CPU::in32(_io_port + TRSTATUS + i * TX_BUFS) & OWN));
        //buf->unlock(); // unlocked by handle int of TOK
    }

    return size;
}

void RTL8139::free(Buffer * buf)
{
    delete buf;
}

bool RTL8139::reconfigure(const Configuration * c = 0)
{
    db<RTL8139>(TRC) << "RTL8139::reconfigure(c=" << c << ")" << endl;

    bool ret = false;

    if(!c) {
        db<RTL8139>(TRC) << "RTL8139::reconfigure: reseting!" << endl;
        CPU::int_disable();
        reset();
        new (&_statistics) Statistics; // reset statistics
        CPU::int_enable();
    } else {
        db<RTL8139>(INF) << "RTL8139::reconfigure: configuration = " << *c << ")" << endl;

        if(c->selector & Configuration::ADDRESS) {
            db<RTL8139>(WRN) << "RTL8139::reconfigure: address changed only in the mediator!)" << endl;
            _configuration.address = c->address;
            ret = true;
        }

        if(c->selector & Configuration::TIMER) {
            if(c->parameter) {
                TSC::time_stamp(TSC::time_stamp() + c->parameter);
                ret = true;
            }
            if(c->timer_frequency) {
                db<PCNet32>(WRN) << "PCNet32::reconfigure: timer frequency cannot be changed!)" << endl;
                ret = false;
            }
        }
    }

    return ret;
}

void RTL8139::reset()
{
    db<RTL8139>(TRC) << "RTL8139::reset()" << endl;

    // Power on the NIC
    CPU::out8(_io_port + CONFIG_1, POWER_ON);

    db<RTL8139>(TRC) << "DMA Buffer is at phy " << _dma_buf->phy_address() << " | log " << _dma_buf->log_address() << endl;

    // Reset the device
    CPU::out8(_io_port + CMD, RESET);

    // Wait for RST bit to be low
    while (CPU::in8(_io_port + CMD) & RESET) {}

    // Tell the NIC where the receive buffer starts
    CPU::out32(_io_port + RBSTART, _dma_buf->phy_address());

    // Configure interrupt mask to transmit OK and receive OK events
    CPU::out16(_io_port + IMR, TOK);

    // Configure which packets should be received, don't wrap around buffer
    CPU::out32(_io_port + RCR, ACCEPT_ANY | WRAP);

    // Receive enable & transmit enable
    CPU::out8(_io_port  + CMD, TE| RE);

    // Set MAC address
    for (int i = 0; i < 6; i++) _configuration.address[i] = CPU::in8(_io_port + MAC0_5 + i);
    db<RTL8139>(INF) << "RTL8139::reset: MAC=" << _configuration.address << endl;
    db<RTL8139>(TRC) << "RBSTART is " << CPU::in32(_io_port + RBSTART) << endl;

}

void RTL8139::handle_int()
{
    TSC::Time_Stamp ts = (Buffer::Metadata::collect_sfdts) ? TSC::time_stamp() : 0;

    /* An interrupt usually means a single frame was transmitted or received,
     * but in certain cases it might handle several frames or none at all.
     */
    unsigned short status = CPU::in16(_io_port + ISR);
    // Acknowledge interrupt
    CPU::out16(_io_port + ISR, status);
    db<RTL8139>(TRC) << "INTERRUPT STATUS " << status << endl;

    if(status & TOK) {
        // Transmit completed successfully, release descriptor
        db<RTL8139>(TRC) << "TOK" << endl;
        //bool transmitted = false;
        for (unsigned char i = 0; i < TX_BUFS; i++) {
            // While descriptors have TOK status, release and advance tail
            unsigned int status_tx = CPU::in32(_io_port + TRSTATUS + _tx_cur_nic * TX_BUFS);
            if(status_tx & STATUS_TOK) {
                _tx_buffer[_tx_cur_nic]->unlock();

                // Clear TOK status
                CPU::out32(_io_port + TRSTATUS + _tx_cur_nic * TX_BUFS, status_tx & ~STATUS_TOK);

                db<RTL8139>(TRC) << "\tTOK unlocked " << _tx_cur_nic << endl;
                _tx_cur_nic = (_tx_cur_nic + 1) % TX_BUFS;
                //transmitted = true;
            } else break;
        }
    }

    if(status & RX_OVERFLOW) {
        db<RTL8139>(WRN) << "\tOVERFLOW in RX!" << endl;
        CPU::out16(_io_port + ISR, RX_OVERFLOW);

    }

    if((status & ROK) && CPU::in16(_io_port + IMR) & ROK) {
        // NIC received frame(s)
        db<RTL8139>(TRC) << "ROK" << endl;

        while ((CPU::in16(_io_port + CBR) < _rx_read) || 
            ((_rx_read + sizeof(Frame)) < CPU::in16(_io_port + CBR)) // while there is pending receive packets
            ) {
            db<RTL8139>(TRC) << "CBR=" << (CPU::in16(_io_port + CBR)) << ",rx=" << _rx_read << endl;
            unsigned int * rx = (unsigned int *) (_rx_buffer + _rx_read);
            db<RTL8139>(TRC) << "rx=" << rx << endl;
            unsigned int header = *rx;
            db<RTL8139>(TRC) << "header=" << hex << header << endl;
            unsigned int packet_len = (header >> 16);
            unsigned int status = header & 0xffff;
            rx++;

            db<RTL8139>(TRC) << "packet_len=" << packet_len << endl;
            db<RTL8139>(TRC) << "status=" << status << endl;

            Frame * frame = reinterpret_cast<Frame *>(rx);
            db<RTL8139>(TRC) << "frame src " << frame->src() << endl;
            Buffer * buf = new (SYSTEM) Buffer(this, rx);
            memcpy(buf->frame(), frame, sizeof(Frame));
            db<RTL8139>(TRC) << "buff src " << buf->frame()->src() << endl;

            buf->sfdts = ts;
            IC::disable(IC::irq2int(_irq));
            if(!notify(buf->frame()->prot(), buf))
                free(buf);

            // update CAPR
            _rx_read += packet_len + 4;
            _rx_read =  (_rx_read + 3) & ~3;

            if(_rx_read > RX_NO_WRAP_SIZE) {
                _rx_read -= RX_NO_WRAP_SIZE;
                db<RTL8139>(TRC) << "\tWRAPPED " << _rx_read << endl;
            }

            _statistics.rx_packets++;
            _statistics.rx_bytes += packet_len;
            CPU::out16(_io_port + CAPR, _rx_read - 0x10);
            IC::enable(IC::irq2int(_irq));
        }

    }
}


void RTL8139::int_handler(IC::Interrupt_Id interrupt)
{
    RTL8139 * dev = get_by_interrupt(interrupt);

    db<RTL8139>(TRC) << "RTL8139::int_handler(int=" << interrupt << ",dev=" << dev << ")" << endl;

    if(!dev)
        db<RTL8139>(WRN) << "RTL8139::int_handler: handler not assigned!" << endl;
    else
        dev->handle_int();
}

__END_SYS
