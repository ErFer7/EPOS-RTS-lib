// EPOS PC Intel PRO/100 (i82559) Ethernet NIC Mediator Initialization

#include <machine/machine.h>
#include <machine/pci.h>
#include <machine/pc/pc_e100.h>
#include <system.h>
#include <memory.h>

__BEGIN_SYS

E100::E100(unsigned int unit, const Log_Addr & io_mem, const IO_Irq & irq, DMA_Buffer * dma_buf)
{
    db<E100>(TRC) << "E100(unit=" << unit << ",io=" << io_mem << ",irq=" << irq << ",dma=" << dma_buf << ")" << endl;

    _configuration.unit = unit;
    _configuration.timer_frequency = TSC::frequency();
    _configuration.timer_accuracy = TSC::accuracy() / 1000; // PPB -> PPM
    if(!_configuration.timer_accuracy)
    	_configuration.timer_accuracy = 1;

    _io_mem = io_mem;
    _irq = irq;
    _csr = static_cast<CSR_Desc *>(io_mem);
    _dma_buffer = dma_buf;

    // Distribute the DMA_Buffer allocated by init()
    Log_Addr log = dma_buf->log_address();
    Phy_Addr phy = dma_buf->phy_address();

    // Initialization Block
    _configCB_phy = phy;
    configCB = log;
    log += align128(sizeof(ConfigureCB));
    phy += align128(sizeof(ConfigureCB));

    _macAddrCB_phy = phy;
    macAddrCB = log;
    log += align128(sizeof(macAddrCB));
    phy += align128(sizeof(macAddrCB));

    _dmadump_phy = phy;
    dmadump = log;
    log += align128(sizeof(struct mem));
    phy += align128(sizeof(struct mem));

    // Rx_Desc Ring
    _rx_cur = 0;
    _rx_last_el = RX_BUFS - 1;
    _rx_ring = log;
    _rx_ring_phy = phy;

    db<E100> (TRC) << "E100(_rx_ring malloc of " << RX_BUFS << " units)" << endl;

    // Rx (RFDs)
    unsigned int i;
    for(i = 0; i < RX_BUFS; i++) {
        log += align128(sizeof(Rx_Desc));
        phy += align128(sizeof(Rx_Desc));
        _rx_ring[i].command = 0;
        _rx_ring[i].size = sizeof(Frame);
        _rx_ring[i].status = Rx_RFD_AVAILABLE;
        _rx_ring[i].actual_count = 0;
        _rx_ring[i].link = phy; //next RFD
    }
    _rx_ring[i-1].command = cb_el;
    _rx_ring[i-1].link = _rx_ring_phy;

    // Tx_Desc Ring
    _tx_cur = 1;
    _tx_prev = 0;
    _tx_ring = log;
    _tx_ring_phy = phy;

    db<E100> (TRC) << "E100(_tx_ring malloc of " << TX_BUFS << " units)" << endl;
    // TxCBs
    for(i = 0; i < TX_BUFS; i++) {
        log += align128(sizeof(Tx_Desc));
        phy += align128(sizeof(Tx_Desc));

        new (&_tx_ring[i]) Tx_Desc(phy);
    }
    _tx_ring[i-1].link = _tx_ring_phy;


    // Rx Buffer
    for(i = 0; i < RX_BUFS; i++) {
        _rx_buffer[i] = new (log) Buffer(this, &_rx_ring[i]);

        log += align128(sizeof(Buffer));
        phy += align128(sizeof(Buffer));
    }

    // Tx Buffer
    for(i = 0; i < TX_BUFS; i++) {
        _tx_buffer[i] = new (log) Buffer(this, &_tx_ring[i]);

        log += align128(sizeof(Buffer));
        phy += align128(sizeof(Buffer));
    }

    _tx_buffer_prev = _tx_buffer[_tx_prev];

    // reset
    reset();
}

void E100::init(unsigned int unit)
{
    db<Init, E100>(TRC) << "E100::init(unit=" << unit << ")" << endl;

    assert(unit < UNITS);

    // Scan the PCI bus for device
    PCI::Locator loc = PCI::scan(PCI_VENDOR_ID, PCI_DEVICE_ID, unit);
    if(!loc) {
        db<Init, E100>(WRN) << "E100::init: PCI scan failed!" << endl;
        return;
    }

    // Try to enable IO regions and bus master
    PCI::command(loc, PCI::command(loc) | PCI::COMMAND_MEMORY | PCI::COMMAND_MASTER);

    // Get the config space header and check it we got MEMORY and MASTER
    PCI::Header hdr;
    PCI::header(loc, &hdr);
    if(!hdr) {
        db<Init, E100>(WRN) << "E100::init: PCI header failed!" << endl;
        return;
    }
    db<Init, E100>(INF) << "E100::init: PCI header=" << hdr << endl;
    if(!(hdr.command & PCI::COMMAND_MEMORY))
        db<Init, E100>(WRN) << "E100::init: I/O memory unaccessible!" << endl;
    if(!(hdr.command & PCI::COMMAND_MASTER))
        db<Init, E100>(WRN) << "E100::init: not master capable!" << endl;

    // Get I/O base port
    Log_Addr io_mem = hdr.region[PCI_REG_MEM].log_addr;

    db<Init, E100>(INF) << "E100::init: I/O memory at "
                        << hdr.region[PCI_REG_MEM].phy_addr
                        << " mapped to "
                        << hdr.region[PCI_REG_MEM].log_addr
                        << " io_mem=" << io_mem << endl;

    // Get I/O irq
    IO_Irq irq = hdr.interrupt_line;
    db<Init, E100>(INF) << "E100::init: PCI interrut pin "
                        << hdr.interrupt_pin << " routed to IRQ "
                        << hdr.interrupt_line << endl;

    // Allocate a DMA Buffer for init block, rx and tx rings
    DMA_Buffer * dma_buf = new (SYSTEM) DMA_Buffer(DMA_BUFFER_SIZE);
    db<Init, E100>(INF) << "E100::init: DMA_Buffer=" << reinterpret_cast<void *>(dma_buf) << " : " << *dma_buf << endl;

    // Initialize the device
    E100 * dev = new (SYSTEM) E100(unit, io_mem, irq, dma_buf);

    // Register the device
    _devices[unit].device = dev;
    _devices[unit].interrupt = IC::irq2int(irq);

    db<E100>(INF) << "E100::init: interrupt: " << _devices[unit].interrupt << ", irq: " << irq << endl;

    // Install interrupt handler
    IC::int_vector(_devices[unit].interrupt, &int_handler);

    // Enable interrupts for device
    IC::enable(_devices[unit].interrupt);
}

__END_SYS
