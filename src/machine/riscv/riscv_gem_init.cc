#include <system/config.h>

#include <machine/machine.h>
#include <machine/riscv/riscv_gem.h>
#include <system.h>

__BEGIN_SYS

GEM::GEM(unsigned int unit, DMA_Buffer* dma_buf)
{
    db<GEM>(TRC) << "GEM(dma=" << dma_buf << ")" << endl;

    _configuration.unit = unit;
    _configuration.timer_frequency = TSC::frequency();
    _configuration.timer_accuracy = TSC::accuracy() / 1000; // PPB -> PPM
    if (!_configuration.timer_accuracy)
        _configuration.timer_accuracy = 1;

    _dma_buf = dma_buf;

    // Distribute the DMA_Buffer allocated by init()
    Log_Addr log = _dma_buf->log_address();
    Phy_Addr phy = _dma_buf->phy_address();

    // Rx_Desc Ring
    _rx_cur = 0;
    _rx_ring = log;
    _rx_ring_phy = phy;
    log += RX_BUFS * align64(sizeof(Rx_Desc));
    phy += RX_BUFS * align64(sizeof(Rx_Desc));

    // Tx_Desc Ring
    _tx_cur = 0;
    _tx_ring = log;
    _tx_ring_phy = phy;
    log += TX_BUFS * align64(sizeof(Tx_Desc));
    phy += TX_BUFS * align64(sizeof(Tx_Desc));

    // Rx_Buffer Ring
    for (unsigned int i = 0; i < RX_BUFS; i++)
    {
        _rx_buffer[i] = new (log) Buffer(this, &_rx_ring[i]);
        _rx_ring[i].addr = phy;    // Keep bits [1-0] from the existing value, and combine with bits [31-2] from buffer addr, manual says do this
        _rx_ring[i].addr &= ~Rx_Desc::OWN; // Owned by NIC
        _rx_ring[i].ctrl = 0;

        log += align64(sizeof(Buffer));
        phy += align64(sizeof(Buffer));
    }
    _rx_ring[RX_BUFS - 1].addr |= Rx_Desc::WRAP; // Mark the last descriptor in the buffer descriptor list with the wrap bit, (bit [1] in word [0]) set.

    // Tx_Buffer Ring
    for (unsigned int i = 0; i < TX_BUFS; i++)
    {
        _tx_buffer[i] = new (log) Buffer(this, &_tx_ring[i]);
        _tx_ring[i].addr = phy;
        _tx_ring[i].ctrl |= Tx_Desc::OWN; // Owned by host

        log += align64(sizeof(Buffer));
        phy += align64(sizeof(Buffer));
    }
    _tx_ring[TX_BUFS - 1].ctrl |= Tx_Desc::WRAP; // Mark the last descriptor in the list with the wrap bit. Set bit [30] in word [1] to 1

    // Configure the NIC
    _configuration.address = mac_address();
    configure(_configuration.address, _tx_ring_phy, _rx_ring_phy);

    dma_rx_buffer_size(sizeof(Frame) + sizeof(Header));
    mode(promiscuous);
}

void GEM::init(unsigned int unit)
{
    db<Init, GEM>(TRC) << "GEM::init()" << endl;

    // Allocate a DMA Buffer for rx and tx rings
    DMA_Buffer* dma_buf = new (SYSTEM) DMA_Buffer(DMA_BUFFER_SIZE);

    // Initialize the device
    GEM* dev = new (SYSTEM) GEM(unit, dma_buf);

    // Register the device
    GEM::_device.device = dev;
    GEM::_device.interrupt = IC::INT_ETH0;

    // Install interrupt handler
    IC::int_vector(IC::INT_ETH0, &int_handler);

    // Enable interrupts for device
    IC::enable(IC::INT_ETH0);

}

__END_SYS