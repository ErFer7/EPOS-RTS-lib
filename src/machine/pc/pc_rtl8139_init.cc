/*
* EPOS RTL8139 PC Ethernet NIC Mediator Implementation
* Authors: Gabriel Müller, Arthur Bridi Guazzelli and Juliana Silva Pinheiro
* Year: 2019
*/

#include <machine/machine.h>
#include <machine/pci.h>
#include <machine/pc/pc_rtl8139.h>
#include <system.h>

__BEGIN_SYS

RTL8139::RTL8139(unsigned int unit, IO_Port io_port, IO_Irq irq, DMA_Buffer * dma_buf)
{
    db<RTL8139>(TRC) << "RTL8139(unit=" << unit << ",io=" << io_port << ",irq=" << irq << ",dma=" << dma_buf << ")" << endl;

    _configuration.unit = unit;
    _configuration.timer_frequency = TSC::frequency();
    _configuration.timer_accuracy = TSC::accuracy() / 1000; // PPB -> PPM
    if(!_configuration.timer_accuracy)
    	_configuration.timer_accuracy = 1;

    _io_port = io_port;
    _irq = irq;
    _dma_buf = dma_buf;

    // Distribute the DMA_Buffer allocated by init()
    Log_Addr log = _dma_buf->log_address();
    Phy_Addr phy = _dma_buf->phy_address();

    // Allocate receive buffer
    _rx_buffer = new (log) char[RX_BUFFER_SIZE];
    _rx_base_phy = phy;

    log += RX_BUFFER_SIZE;
    phy += RX_BUFFER_SIZE;

    // Allocate four transmit buffers
    for (unsigned int i = 0; i < TX_BUFS; i++) {
        _tx_base_phy[i] = phy;
        _tx_buffer[i] = new (log) Buffer(this, &_tx_base_phy[i]);
        log += sizeof(Buffer);
        phy += sizeof(Buffer);
    }

    // Reset device
    reset();
}

void RTL8139::init(unsigned int unit)
{
    db<Init, RTL8139>(TRC) << "DMA_BUFF_SIZE " << DMA_BUFFER_SIZE<< endl;
    // Scan the PCI bus for device
    PCI::Locator loc = PCI::scan(PCI_VENDOR_ID, PCI_DEVICE_ID, unit);
    if(!loc) {
        db<Init, RTL8139>(WRN) << "RTL8139::init: PCI scan failed!" << endl;
        return;
    }

    // Try to enable IO regions and bus master
    PCI::command(loc, PCI::command(loc) | PCI::COMMAND_IO | PCI::COMMAND_MASTER);

    // Get the config space header and check if we got IO and MASTER
    PCI::Header hdr;
    PCI::header(loc, &hdr);
    if(!hdr) {
        db<Init, RTL8139>(WRN) << "RTL8139::init: PCI header failed!" << endl;
        return;
    }
    db<Init, RTL8139>(INF) << "RTL8139::init: PCI header=" << hdr << endl;
    if(!(hdr.command & PCI::COMMAND_IO))
        db<Init, RTL8139>(WRN) << "RTL8139::init: I/O unaccessible!" << endl;
    if(!(hdr.command & PCI::COMMAND_MASTER))
        db<Init, RTL8139>(WRN) << "RTL8139::init: not master capable!" << endl;

    // Get I/O base port
    IO_Port io_port = hdr.region[PCI_REG_IO].phy_addr;
    db<Init, RTL8139>(INF) << "RTL8139::init: I/O port at " << (void *)(int)io_port << endl;

    // Get I/O irq
    IO_Irq irq = hdr.interrupt_line;
    db<Init, RTL8139>(TRC) << "RTL8139::init: PCI interrut pin " << hdr.interrupt_pin << " routed to IRQ " << hdr.interrupt_line << endl;

    // Allocate a DMA Buffer for init block, rx and tx rings
    DMA_Buffer * dma_buf = new (SYSTEM) DMA_Buffer(DMA_BUFFER_SIZE);

    // Initialize the device
    RTL8139 * dev = new (SYSTEM) RTL8139(unit, io_port, irq, dma_buf);

    // Register the device
    _devices[unit].device = dev;
    _devices[unit].interrupt = IC::irq2int(irq);

    // Install interrupt handler
    IC::int_vector(_devices[unit].interrupt, &int_handler);

    // Enable interrupts for device
    IC::enable(_devices[unit].interrupt);

}
__END_SYS
