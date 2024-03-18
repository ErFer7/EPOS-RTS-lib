// EPOS NIC Test Programs

#include <machine/nic.h>
#include <time.h>
#include <utility/random.h>

using namespace EPOS;

OStream cout;

template<typename Family>
class NIC_Receiver: public NIC<Family>::Observer
{
private:
    void update(typename NIC<Family>::Observed * obs,  const typename NIC<Family>::Protocol & p, typename NIC<Family>::Buffer * buf) {
        typename Family::Frame * frame = buf->frame();
        char * data = frame->template data<char>();
        for(unsigned int i = 0; i < buf->size() - sizeof(typename Family::Header) - sizeof(typename Family::Trailer); i++)
            cout << data[i];
        cout << "\n" << endl;
        buf->nic()->free(buf); // to return to the buffer pool;
    }
};

#ifdef __ethernet__

void ethernet_test() {
    cout << "Ethernet Test" << endl;

    NIC<Ethernet> * nic = Traits<Ethernet>::DEVICES::Get<0>::Result::get(0);

    NIC<Ethernet>::Address src, dst;
    NIC<Ethernet>::Protocol prot;
    char data[nic->mtu()];

    NIC<Ethernet>::Address self = nic->address();
    cout << "  MAC: " << self << endl;

    if(self[5] % 2) { // sender
        Delay (5000000);

        for(int i = 0; i < 10; i++) {
            memset(data, '0' + i, nic->mtu());
            data[nic->mtu() - 1] = '\n';
            nic->send(nic->broadcast(), 0x8888, data, nic->mtu());
        }
    } else { // receiver
        for(int i = 0; i < 10; i++) {
           nic->receive(&src, &prot, data, nic->mtu());
           cout << "  Data: " << data;
        }
    }

    NIC<Ethernet>::Statistics stat = nic->statistics();
    cout << "Statistics\n"
         << "Tx Packets: " << stat.tx_packets << "\n"
         << "Tx Bytes:   " << stat.tx_bytes << "\n"
         << "Rx Packets: " << stat.rx_packets << "\n"
         << "Rx Bytes:   " << stat.rx_bytes << "\n";
}

#endif

#ifdef __ieee802_15_4__

void ieee802_15_4_test() {
    cout << "IEEE 802.15.4 Test" << endl;

    NIC<IEEE802_15_4> * nic = Traits<IEEE802_15_4>::DEVICES::Get<0>::Result::get(0);

    NIC_Receiver<IEEE802_15_4> * receiver = new NIC_Receiver<IEEE802_15_4>;
    nic->attach(receiver, IEEE802_15_4::PROTO_ELP);

    char original[] = "000 ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890\0";
    char data[150];

    NIC<IEEE802_15_4>::Address self = nic->address();
    cout << "  MAC: " << self << endl;

    int pct = 0;

    while(true) {
        for(int i = 0; i < 10; i++) {
            int rnd = Random::random();
            if(rnd < 0)
                rnd = -rnd;

            int size = 12 + (rnd % 100);
            for(int p = 0; p < size; p++)
                data[p] = original[p];
            data[size-3] = '='; // to easily verify if received correctly
            data[size-2] = 'X'; // Is sent raw, the last 2 bytes are CRC, must be included in "Packet", and
            data[size-1] = 'X'; // will be discarded at receiver.
            data[0] = '0' + pct / 100;
            data[1] = '0' + (pct % 100) / 10;
            data[2] = '0' + pct % 10;
            pct = (pct + 1) % 1000;

            nic->send(nic->broadcast(), IEEE802_15_4::PROTO_ELP, data, size);
            Alarm::delay(1000000);
        }

        NIC<IEEE802_15_4>::Statistics stat = nic->statistics();
        cout << "Statistics\n"
             << "Tx Packets: " << stat.tx_packets << "\n"
             << "Tx Bytes:   " << stat.tx_bytes << "\n"
             << "Rx Packets: " << stat.rx_packets << "\n"
             << "Rx Bytes:   " << stat.rx_bytes << "\n";
    }
}

#endif

int main()
{
    cout << "NIC Test" << endl;

#ifdef __ethernet__
    ethernet_test();
#else
    cout << "Ethernet not configured, skipping test!" << endl;
#endif

#ifdef __ieee802_15_4__
    ieee802_15_4_test();
#else
    cout << "IEEE 802.15.4 not configured, skipping test!" << endl;
#endif

    return 0;
}
