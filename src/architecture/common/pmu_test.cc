#include <utility/ostream.h>
#include <architecture.h>
#include <time.h>

using namespace EPOS;

OStream cout;

void print_channels()
{
    cout << "Unhalted CPU cycles: " << PMU::read(0)
         << "\nCPU cycles: " << PMU::read(1)
         << "\nInstrctions: " << PMU::read(2)
         << "\nExceptions: " << PMU::read(3)
         << "\nInterrupts: " << PMU::read(4) << endl;
}

void do_something()
{
    int j = 2;

    for(int i = 0; i < 100; i++){
        j = j + j;
        if(j < 0)
            j = 1;
    }
}

int main()
{
    cout << "PMU Test: " << endl;

    cout << "Configuring counters for the following events: Unhalted CPU Cycles, CPU Cycles, Retired Instructions, Exceptions Taken, and Interrupts taken." << endl;;
    PMU::config(0, PMU::UNHALTED_CYCLES);
    PMU::config(1, PMU::CPU_CYCLES);
    PMU::config(2, PMU::INSTRUCTIONS_RETIRED);
    PMU::config(3, PMU::EXCEPTIONS);
    PMU::config(4, PMU::INTERRUPTS);
    print_channels();

    cout << "\nRunning a couple of instructions: ";
    do_something();
    cout << " done!" << endl;
    print_channels();

    cout << "\nStopping counters and running another couple of instructions: ";
    PMU::stop(0);
    PMU::stop(1);
    PMU::stop(2);
    PMU::stop(3);
    PMU::stop(4);
    do_something();
    cout << " done!" << endl;
    print_channels();

    cout << "\nRestarting counters and running another couple of instructions:";
    PMU::start(0);
    PMU::start(1);
    PMU::start(2);
    PMU::start(3);
    PMU::start(4);
    do_something();
    cout << " done!" << endl;
    print_channels();

    cout << "\nResetting counters: ";
    PMU::reset(0);
    PMU::reset(1);
    PMU::reset(2);
    PMU::reset(3);
    PMU::reset(4);
    cout << " done!" << endl;
    print_channels();

    return 0;
}
