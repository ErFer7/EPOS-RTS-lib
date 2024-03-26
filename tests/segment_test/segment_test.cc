// EPOS Segment Test Program

#include <memory.h>

using namespace EPOS;

#ifdef __cortex_m__
const unsigned ES1_SIZE = 100;
const unsigned ES2_SIZE = 200;
#else
const unsigned ES1_SIZE = 10000;
const unsigned ES2_SIZE = 100000;
#endif

int main()
{
    OStream cout;

    cout << "Segment test" << endl;

    if(Traits<Build>::MODEL == Traits<Build>::SiFive_E) {
        cout << "This test requires multiheap and the SiFive-E doesn't have enough memory to run it!" << endl;
        return 0;
    }

    cout << "My address space's page directory is located at " << reinterpret_cast<void *>(MMU::current()) << "" << endl;
    Address_Space * as = new (SYSTEM) Address_Space(MMU::current());

    cout << "Creating two extra data segments:" << endl;

    Segment * es1 = new (SYSTEM) Segment(ES1_SIZE, MMU::Flags::SYSC);
    Segment * es2 = new (SYSTEM) Segment(ES2_SIZE, MMU::Flags::SYSD);

    cout << "  extra segment 1 => " << ES1_SIZE << " bytes, done!" << endl;
    cout << "  extra segment 2 => " << ES2_SIZE << " bytes, done!" << endl;

    cout << "Attaching segments:" << endl;
    CPU::Log_Addr * extra1 = as->attach(es1);
    CPU::Log_Addr * extra2 = as->attach(es2);
    cout << "  extra segment 1 => " << extra1 << " done!" << endl;
    cout << "  extra segment 2 => " << extra2 << " done!" << endl;

    cout << "Clearing segments:";
    memset(extra1, 0, ES1_SIZE);
    memset(extra2, 0, ES2_SIZE);
    cout << "  done!" << endl;

    cout << "Detaching segments:";
    as->detach(es1);
    as->detach(es2);
    cout << "  done!" << endl;

    cout << "Deleting segments:";
    delete es1;
    delete es2;
    cout << "  done!" << endl;

    cout << "I'm done, bye!" << endl;

    return 0;
}
