// EPOS Priority Inversion Test Program

#include <time.h>
#include <process.h>
#include <real-time.h>
#include <synchronizer.h>

using namespace EPOS;


// Por enquanto manter todos iguais, menos activation e criterion;
const unsigned int iterations = 50;
const unsigned int period = 50; // ms
const unsigned int wcet = 20; //ms
const unsigned int wait_time = 50; // ms NÃO FUNCIONOU?


OStream cout;
Chronometer chrono;
Mutex criticalSection;

Periodic_Thread * threadL;
Periodic_Thread * threadM;
Periodic_Thread * threadH;

// find high processing function, estimate its time (should be variable) and put the threads to execute.
void heavyWork() {
    Delay working(1500000); // should print if i'm back to work, to make sure priority inversion
    // maybe similar to teachers test? what does wait_next does?
}

void criticalSectionCode(char c, Thread* this_thread) {
    cout << "Thread " << c << " tries to take lock" << endl;
    cout << "[p(" << c << ") = " << this_thread->priority() << "]" << endl;
    criticalSection.lock();

    Microsecond elapsed = chrono.read() / 1000;
    cout << "Thread " << c << " got the lock, " << elapsed << " time has passed" << endl;

    heavyWork();    

    cout << "Thread " << c << " will release the lock and finish" << endl;
    criticalSection.unlock();
}

int lowPriority() { 
    criticalSectionCode('L', threadL);
    return 'L';
}

int mediumPriority() {
    cout << "Thread M started" << endl;
    cout << "[p(M) = " << threadM->priority() << "]" << endl;
    heavyWork();

    Microsecond elapsed = chrono.read() / 1000;
    cout << "Thread M finished " << elapsed << " has passed" << endl;
    return 'M';
}

int highPriority() { 
    criticalSectionCode('H', threadH);
    return 'H';
}

int main() {
    cout << "Priority Inversion Test" << endl;

    // INITIAL STATE:
    // Thread L should start, acquire the lock and start heavy work.
    // Thread H can't start work because is waiting for lock
    // Thread M should preempt L and delay the lock release from H
    // So Thread M has "higher priority" than H without being in the critical section, that should not happen


    // Understand the parameters to make priorities L < M < H
    threadL = new Periodic_Thread(RTConf(period * 1000, 0, wcet * 1000, 0, iterations, Thread::READY, Thread::LOW), &lowPriority);
    Delay pickLockFirst(100000); // to make sure L starts first, wait time on parameters did not work????
    threadH = new Periodic_Thread(RTConf(period * 1000, 0, wcet * 1000, wait_time * 1000, iterations, Thread::READY, Thread::HIGH), &highPriority);
    threadM = new Periodic_Thread(RTConf(period * 1000, 0, wcet * 1000, wait_time * 1000, iterations, Thread:: READY, Thread::NORMAL - 1), &mediumPriority);
    // Normal - 1 pq acho que se for == NORMAL ele faz o cálculo pelo Criterion -> LLF()

    chrono.start();

    threadL->join();
    threadH->join();
    threadM->join();

    chrono.stop();

    return 0;
}