// EPOS Priority Inversion Test Program

#include <time.h>
#include <real-time.h>
#include <synchronizer.h>

using namespace EPOS;

OStream cout;
Chronometer chrono;
Mutex criticalSection;

Periodic_Thread * threadL;
Periodic_Thread * threadM;
Periodic_Thread * threadH;


void criticalSectionCode(char c) {
    cout << "Thread " << c << "tries to take lock"
    criticalSection.lock();

    Microsecond elapsed = chrono.read() / 1000;
    cout << "Thread " << c << "got the lock, " << elapsed << "time has passed";

    // TODO: do some heavy work:
    // find high processing function, estimate its time (should be variable) and put the threads to execute.

    criticalSection.unlock();
}

void lowPriority() { criticalSectionCode('L') }

void mediumPriority() {
    cout << "Thread M started";

    // TODO: do some heavy work

    Microsecond elapsed = chrono.read() / 1000;
    cout << "Thread M finished " << elapsed << " has passed";

}

void highPriority() {
    criticalSectionCode('H')
}

int main() {
    cout << "Priority Inversion Test" << endl;

    // INITIAL STATE:
    // Thread L should start, acquire the lock and start heavy work.
    // Thread H can't start work because is waiting for lock
    // Thread M should preempt L and delay the lock release from H
    // So Thread M has "higher priority" than H without being in the critical section, that should not happen


    // Understand the parameters to make priorities L < M < H
    threadL = new Periodic_Thread(RTConf(), &lowPriority);
    threadH = new Periodic_Thread(RTConf(), &highPriority);
    threadM = new Periodic_Thread(RTConf(), &mediumPriority);

    chrono.start();

    threadL.join();
    threadH.join();
    threadM.join();

    chrono.stop();

    return 0;
}