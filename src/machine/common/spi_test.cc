// EPOS SPI Test

#include <machine/spi.h>
#include <utility/string.h>

using namespace EPOS;

OStream cout;

int test_number = 0;

int main()
{
    cout << "SPI Test" << endl;

    cout << "Configuring SPI[0] to a single lane, in Master mode at 250 kbps with 8-bit data: " << endl;
    SPI spi(0, SPI::Si5_SINGLE, SPI::MASTER, 250000, 8);

    SPI::Protocol protocol;
    SPI::Mode mode;
    unsigned int bit_rate;
    unsigned int data_bits;
    spi.config(&protocol, &mode, &bit_rate, &data_bits);
    cout << "  protocol=" << protocol << ", mode=" << mode << ", clock=" << Traits<SPI>::CLOCK << ", bit_rate=" << bit_rate << ", data_bits=" << data_bits << endl;

    char buf[] = "A dummy string just to fill the FIFO";
    spi.write(buf, sizeof(buf));
    cout << buf << endl;

    spi.read(buf, sizeof(buf));
    cout << buf << endl;

    return 0;
}
